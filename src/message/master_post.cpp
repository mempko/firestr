/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#include "util/log.hpp"

#include <sstream>

namespace fire
{
    namespace message
    {
        namespace n = fire::network;
        namespace u = fire::util;

        namespace
        {
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
                n::endpoint ep;
                if(!o->_connections.receive(ep, data))
                {
                    u::sleep_thread(THREAD_SLEEP);
                    continue;
                }
                if(o->_outside_stats.on) o->_outside_stats.in_push_count++;


                //parse message
                std::stringstream s(u::to_str(data));
                message m;
                s >> m;

                //insert the from_ip, from_port
                m.meta.extra["from_protocol"] = ep.protocol;
                m.meta.extra["from_ip"] = ep.address;
                m.meta.extra["from_port"] = ep.port;

                //pop off master address
                m.meta.to.pop_front();

                //send message to interal component
                o->send(m);
                if(o->_outside_stats.on) o->_outside_stats.in_pop_count++;
            }
            catch(std::exception& e)
            {
                LOG << "error recieving message: " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "error recieving message: unknown error." << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: master_post::in_thread" << std::endl;
        }

        void out_thread(master_post_office* o)
        try
        {
            REQUIRE(o);

            std::string last_address;

            bool sent = false;
            while(!o->_done || sent)
            try
            {
                sent = false;

                message m;
                if(!o->_out.pop(m))
                {
                    u::sleep_thread(THREAD_SLEEP);
                    continue;
                }

                sent = true;

                REQUIRE_GREATER_EQUAL(m.meta.from.size(), 1);
                REQUIRE_GREATER_EQUAL(m.meta.to.size(), 1);

                const std::string outside_queue_address = m.meta.to.front();
                last_address = outside_queue_address;

                std::stringstream s;
                s << m;
                u::bytes data = u::to_bytes(s.str());

                o->_connections.send(outside_queue_address, data);

                if(o->_outside_stats.on) o->_outside_stats.out_pop_count++;
            }
            catch(std::exception& e)
            {
                LOG << "error sending message to " << last_address << ": " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "error sending message to " << last_address << ": unknown error." << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: master_post::out_thread" << std::endl;
        }

        master_post_office::master_post_office(
                const std::string& in_host,
                const std::string& in_port) : 
            _in_host{in_host},
            _in_port{in_port},
            _connections{POOL_SIZE, in_port}
        {
            _address = n::make_udp_address(_in_host,_in_port);

            _in_thread.reset(new std::thread{in_thread, this});
            _out_thread.reset(new std::thread{out_thread, this});

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
            if(_outside_stats.on) _outside_stats.out_push_count++;
            _out.push(m);
            return true;
        }
    }
}
