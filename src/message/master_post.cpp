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
#include "message/master_post.hpp"
#include "network/boost_asio.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace fire
{
    namespace message
    {
        namespace n = fire::network;
        namespace u = fire::util;

        namespace
        {
            const double STUN_WAIT = 10; //in milliseconds 
            const double THREAD_SLEEP = 10; //in milliseconds 
            const size_t POOL_SIZE = 20; //small pool size for now
        }

        void in_thread(master_post_office* o)
        try
        {
            REQUIRE(o);

            while(!o->_done)
            try
            {
                //get data from outside world
                u::bytes data;
                if(!o->_in->recieve(data))
                {
                    u::sleep_thread(THREAD_SLEEP);
                    continue;
                }

                //get socket info of the message just recieved.
                auto socket = o->_in->get_socket_info();

                //parse message
                std::stringstream s(u::to_str(data));
                message m;
                s >> m;

                //insert the from_ip
                m.meta.extra["from_ip"] = socket.remote_address;

                //pop off master address
                m.meta.to.pop_front();

                //send message to interal component
                o->send(m);
            }
            catch(std::exception& e)
            {
                std::cerr << "error recieving message: " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "error recieving message: unknown error." << std::endl;
            }
        }
        catch(...)
        {
            std::cerr << "exit: master_post::in_thread" << std::endl;
        }

        void out_thread(master_post_office* o)
        try
        {
            REQUIRE(o);

            std::string last_address;

            while(!o->_done)
            try
            {
                message m;
                if(!o->_out.pop(m))
                {
                    u::sleep_thread(THREAD_SLEEP);
                    continue;
                }

                REQUIRE_GREATER_EQUAL(m.meta.from.size(), 1);
                REQUIRE_GREATER_EQUAL(m.meta.to.size(), 1);

                const std::string outside_queue_address = m.meta.to.front();
                last_address = outside_queue_address;

                //get output queue from connection pool or connect
                auto out = o->_connections.get(outside_queue_address);
                if(!out) out = o->_connections.connect(outside_queue_address);

                CHECK(out);

                std::stringstream s;
                s << m;
                u::bytes data = u::to_bytes(s.str());

                out->send(data);
            }
            catch(std::exception& e)
            {
                std::cerr << "error sending message to " << last_address << ": " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "error sending message to " << last_address << ": unknown error." << std::endl;
            }
        }
        catch(...)
        {
            std::cerr << "exit: master_post::out_thread" << std::endl;
        }

        void master_post_office::setup_input_connection()
        {
            auto address = n::make_tcp_address("*", _in_port);

            n::queue_options qo = { 
                {"bnd", "1"},
                {"block", "0"},
                {"track_incoming", "1"}};

            _in = n::create_message_queue(address, qo);
        }

        master_post_office::master_post_office(
                const std::string& in_host,
                const std::string& in_port) : 
            _in_host{in_host},
            _in_port{in_port},
            _connections{POOL_SIZE, in_port}
        {
            //setup outside address
            _address = n::make_tcp_address(_in_host,_in_port);

            setup_input_connection();

            //here we use the magic * 
            _in_thread.reset(new std::thread{in_thread, this});
            _out_thread.reset(new std::thread{out_thread, this});

            ENSURE(_in);
            ENSURE(_in_thread);
            ENSURE(_out_thread);
            ENSURE_FALSE(_address.empty());
        }

        master_post_office::~master_post_office()
        {
            INVARIANT(_in_thread);
            INVARIANT(_out_thread);

            _done = true;
            _in_thread->join();
            _out_thread->join();
        }

        bool master_post_office::send_outside(const message& m)
        {
            _out.push(m);
            return true;
        }
    }
}
