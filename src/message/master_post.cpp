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

        master_post_office::master_post_office(
                const std::string& in_host,
                const std::string& in_port)
        {
            _address = "zmq,tcp://" + in_host + ":" + in_port;

            n::queue_options qo = { 
                {"pul", "1"}, 
                {"bnd", "1"},
                {"block", "0"}};

            _in = n::create_message_queue(_address, qo);
            _in_thread.reset(new std::thread{in_thread, this});

            ENSURE(_in);
            ENSURE(_in_thread);
            ENSURE_FALSE(_address.empty());
        }

        master_post_office::~master_post_office()
        {
            INVARIANT(_in);

            _done = true;
            _in_thread->join();
        }

        bool master_post_office::send_outside(const message& m)
        {
            REQUIRE_GREATER_EQUAL(m.meta.from.size(), 1);
            REQUIRE_GREATER_EQUAL(m.meta.to.size(), 1);

            const std::string outside_queue_address = m.meta.to.front();

            n::queue_options qo = { 
                {"psh", "1"}, 
                {"con", "1"},
                {"block", "0"}};

            try
            {
                n::message_queue_ptr out = 
                    n::create_message_queue(
                        outside_queue_address, qo);

                CHECK(out);

                std::stringstream s;
                s << m;
                u::bytes data = u::to_bytes(s.str());

                return out->send(data);
            }
            catch(std::exception& e)
            {
                std::cerr << "error sending message: " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "error sending message: unknown error." << std::endl;
            }

            return false;
        }
    }
}
