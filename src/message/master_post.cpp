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
            const double THREAD_SLEEP = 10; //in milliseconds 
        }

        void in_thread(master_post_office* o)
        {
            REQUIRE(o);
            REQUIRE(o->_in);

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


                //parse message
                std::stringstream s(u::to_str(data));
                message m;
                s >> m;

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

        void out_thread(master_post_office* o)
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

                auto c = o->_connections.find(outside_queue_address);

                n::message_queue_ptr out;
                if(c == o->_connections.end())
                {
                    n::queue_options qo = { 
                        {"psh", "1"}, 
                        {"con", "1"},
                        {"block", "0"}};

                    out = n::create_message_queue(outside_queue_address, qo);
                    o->_connections[outside_queue_address] = out;
                }
                else
                {
                    out = c->second;
                }

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

        master_post_office::master_post_office(
                const std::string& in_host,
                const std::string& in_port)
        {
            //setup outside address
            _address = "zmq,tcp://" + in_host + ":" + in_port;

            //here we use the magic * 
            const std::string address = "zmq,tcp://*:" + in_port;

            n::queue_options qo = { 
                {"pul", "1"}, 
                {"bnd", "1"},
                {"threads", "5"},
                {"block", "0"}};

            _in = n::create_message_queue(address, qo);
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
