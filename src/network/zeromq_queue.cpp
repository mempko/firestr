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

#include "network/zeromq_queue.hpp"
#include "util/thread.hpp"

#include <stdexcept>
#include <sstream>
#include <boost/lexical_cast.hpp>

using boost::lexical_cast;
namespace u = fire::util;

namespace fire
{
    namespace network
    {
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

        zmq_params parse_zmq_params(const address_components& c)
        {
            const auto& o = c.options;

            zmq_params p;
            p.type = determine_socket_type(o);
            p.mode = determine_connection_mode(o);
            p.uri = c.location;
            p.threads = get_opt(o, "threads", 1);
            p.block = get_opt(o, "block", 1);
            p.timeout = to_microseconds(get_opt(o, "timeout", 0.0));
            p.wait = get_opt(o, "wait", 0.0);
            p.linger = get_opt(o, "linger", 0);
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

        zmq_queue::zmq_queue(const zmq_params& p) : _p(p)
        {
            _c.reset(new zmq::context_t{p.threads});
            _s = create_socket(p, *_c);
            bind_socket(*_s, p);

            INVARIANT(_c);
            INVARIANT(_s);
        }

        zmq_queue::~zmq_queue()
        {
           if(_p.wait > 0) u::sleep_thread(_p.wait);
        }

        bool zmq_queue::send(const u::bytes& b)
        {
            REQUIRE_FALSE(b.empty());
            INVARIANT(_s);
            INVARIANT(_c);

            if(_p.timeout > 0) 
                if(timedout(ZMQ_POLLOUT)) return false;

            zmq::message_t m(b.size());
            std::copy(b.begin(), b.end(), reinterpret_cast<u::byte*>(m.data()));

            return _s->send(m, _p.block ? 0 : ZMQ_NOBLOCK);
        }

        bool zmq_queue::recieve(u::bytes& b)
        {
            INVARIANT(_s);
            INVARIANT(_c);

            if(_p.timeout > 0) 
                if(timedout(ZMQ_POLLIN)) return false;

            zmq::message_t m;
            if(!_s->recv(&m, _p.block ? 0 : ZMQ_NOBLOCK)) return false;

            b.resize(m.size());
            std::copy(
                    reinterpret_cast<u::byte*>(m.data()), 
                    reinterpret_cast<u::byte*>(m.data()) + m.size(), 
                    b.begin());

            ENSURE_EQUAL(b.size(), m.size());
            return true;
        }

        bool zmq_queue::timedout(short event)
        {
            INVARIANT(_s);
            INVARIANT(_c);

            zmq::pollitem_t i[] = {{*_s, 0, event, 0}};
            int r = zmq::poll(i, 1, _p.timeout);

            return !(i[0].revents & event);
        }

        socket_info zmq_queue::get_socket_info() const 
        {
            return {};
        }

        message_queue_ptr create_zmq_message_queue(const address_components& c)
        {
            zmq_params p = parse_zmq_params(c);
            return message_queue_ptr{new zmq_queue{p}};
        }
    }
}
