/*
 * Copyright (C) 2012  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "network/message_queue.hpp"
#include "util/string.hpp"
#include "util/mencode.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>

#include <zmq.hpp>

using boost::lexical_cast;

namespace fire
{
    namespace network
    {
        namespace
        {
            const std::string SPLIT_CHAR = ",";
        }

        enum message_type {zeromq};
        typedef std::vector<std::string> strings;
        struct address_components
        {
            std::string queue_address; 
            message_type type;
            std::string location;
            queue_options options;
        };

        message_type determine_type(const std::string type)
        {
            message_type t;
            if(type == "zmq") t = zeromq; 
            else std::invalid_argument("Queue type `" + type + "' is not valid. Currently zmq is supported");
            return t;
        }

        queue_options parse_options(const strings& ss)
        {
            REQUIRE_FALSE(ss.empty());
            
            queue_options o;
            for(auto s : ss)
            {
                auto p = util::split<strings>(s, "=");
                if(p.size() == 1) o[p[0]] = "1";
                else if(p.size() == 2) o[p[0]] = p[1];
                else std::invalid_argument("The queue options `" + s + "' is invalid. Cannot have more than one `='"); 
            }

            ENSURE_EQUAL(ss.size(), o.size());
            return o;
        }

        address_components parse_address(
                const std::string& queue_address, 
                const queue_options& defaults)
        {
            auto s = util::split<strings>(queue_address, SPLIT_CHAR);
            if(s.size() < 2) std::invalid_argument("Queue address must have at least a type and location. Example: zmq,http://localhost:10"); 
            address_components c;
            c.queue_address = queue_address;
            c.type = determine_type(s[0]);
            c.location = s[1];
            if(s.size() > 2) 
            {
                auto overrides = parse_options(strings(s.begin() + 2, s.end()));
                c.options = defaults;
                c.options.insert(overrides.begin(), overrides.end());
            }

            return c;
        }

        message_queue_ptr create_zmq_message_queue(const address_components& c);

        message_queue_ptr create_message_queue(
                const std::string& queue_address, 
                const queue_options& defaults)
        {
            auto c = parse_address(queue_address, defaults); 
            message_queue_ptr p;

            switch(c.type)
            {
                case zeromq: p = create_zmq_message_queue(c); break;
                default: CHECK(false && "missed case");
            }

            ENSURE(p);
            return p;
        }
        
        typedef std::unique_ptr<zmq::context_t> context_ptr;
        typedef std::unique_ptr<zmq::socket_t> socket_ptr;

        enum socket_type {request, reply, push, pull, publish, subscribe};
        enum connect_mode {bind, connect}; 

        struct zmq_params
        {
            socket_type type;
            connect_mode mode;
            std::string uri;
            int threads;
            bool block;
            size_t timeout;
            double wait;
            int linger;
            boost::uint64_t hwm;
        };

        socket_type determine_socket_type(const queue_options& o)
        {
            socket_type t;
            if(o.count("req")) t = request;
            else if(o.count("rep")) t = reply;
            else if(o.count("psh")) t = push;
            else if(o.count("pul")) t = pull;
            else if(o.count("pub")) t = publish;
            else if(o.count("sub")) t = subscribe;
            else 
                throw std::invalid_argument("cannot find a valid zmq socket type. [req, rep, psh, pul, pub, sub]");
            return t;
        }

        connect_mode determine_connection_mode(const queue_options& o)
        {
            connect_mode m = connect;

            if(o.count("bnd")) m = bind;
            else if(o.count("con")) m = connect;

            return m;
        }

        double to_microseconds(double seconds)
        {
            return seconds*1000000.0;
        }

        template<class t>
            t get_opt(const queue_options& o, const std::string& k, t def)
            {
                auto i = o.find(k);
                if(i != o.end()) return lexical_cast<t>(i->second);
                return def;
            }

        zmq_params parse_zmq_params(const address_components& c)
        {
            const auto& o = c.options;

            zmq_params p;
            p.type = determine_socket_type(o);
            p.mode = determine_connection_mode(o);
            p.uri = c.location;
            p.threads = get_opt(o, "threads", 5);
            p.block = get_opt(o, "block", true);
            p.timeout = to_microseconds(get_opt(o, "timeout", 0.0));
            p.wait = get_opt(o, "wait", 0.0);
            p.linger = get_opt(o, "linger", -1);
            p.hwm = get_opt<boost::uint64_t>(o, "hwm", 0);

            return p;
        }

        socket_ptr create_socket(const zmq_params& p, zmq::context_t& c)
        {
            socket_ptr s;

            switch(p.type)
            {
                case request: s.reset(new zmq::socket_t(c, ZMQ_REQ)); break;
                case reply: s.reset(new zmq::socket_t(c, ZMQ_REP)); break;
                case push: s.reset(new zmq::socket_t(c, ZMQ_PUSH)); break;
                case pull: s.reset(new zmq::socket_t(c, ZMQ_PULL)); break;
                case publish: s.reset(new zmq::socket_t(c, ZMQ_PUB)); break;
                case subscribe: 
                              s.reset(new zmq::socket_t(c, ZMQ_SUB)); 
                              s->setsockopt(ZMQ_SUBSCRIBE, "", 0);
                              break;
                default: CHECK(false && "missed case");
            }

            CHECK(s);

            if(p.linger != -1) s->setsockopt(ZMQ_LINGER, &p.linger, sizeof(p.linger));
            if(p.hwm > 0) s->setsockopt(ZMQ_HWM, &p.hwm, sizeof(p.hwm));

            ENSURE(s)
            return s;
        }

        void bind_socket(zmq::socket_t& s, const zmq_params& p)
        {
            switch(p.mode)
            {
                case bind: s.bind(p.uri.c_str()); break;
                case connect: s.connect(p.uri.c_str()); break;
                default: CHECK(false && "missed case");
            }
        }

        class zmq_queue : public message_queue
        {
            public:
                zmq_queue(const zmq_params& p) : _p{p}
                {
                    _c.reset(new zmq::context_t{p.threads});
                    _s = create_socket(p, *_c);
                    bind_socket(*_s, p);

                    INVARIANT(_c);
                    INVARIANT(_s);
                }

                virtual ~zmq_queue()
                {
                    //thread_sleep(_p.wait)
                }

            public:
                virtual bool send(const util::bytes& b)
                {
                    REQUIRE_FALSE(b.empty());
                    INVARIANT(_s);
                    INVARIANT(_c);

                    if(_p.timeout > 0) 
                        if(timedout(ZMQ_POLLOUT)) return false;

                    zmq::message_t m(b.size());
                    std::copy(b.begin(), b.end(), reinterpret_cast<util::byte*>(m.data()));

                    return _s->send(m, _p.block ? 0 : ZMQ_NOBLOCK);
                }

                virtual bool recieve(util::bytes& b)
                {
                    INVARIANT(_s);
                    INVARIANT(_c);
                    if(_p.timeout > 0) 
                        if(timedout(ZMQ_POLLIN)) return false;

                    zmq::message_t m;
                    if(!_s->recv(&m, _p.block ? 0 : ZMQ_NOBLOCK)) return false;

                    b.resize(m.size());
                    std::copy(
                            reinterpret_cast<util::byte*>(m.data()), 
                            reinterpret_cast<util::byte*>(m.data()) + m.size(), 
                            b.begin());

                    ENSURE_EQUAL(b.size(), m.size());
                    return true;
                }

            private:
                bool timedout(short event)
                {
                    INVARIANT(_s);
                    INVARIANT(_c);

                    zmq::pollitem_t i[] = {{*_s, 0, event, 0}};
                    int r = zmq::poll(i, 1, _p.timeout);

                    return !(i[0].revents & event);
                }

            private:
                zmq_params _p;
                context_ptr _c;
                socket_ptr _s;

        };

        message_queue_ptr create_zmq_message_queue(const address_components& c)
        {
            zmq_params p = parse_zmq_params(c);
            return message_queue_ptr{new zmq_queue{p}};
        }
    }
}
