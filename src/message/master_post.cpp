/*
 * Copyright (C) 2014  Maxim Noah Khailo
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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * Botan library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than Botan. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */
#include "message/master_post.hpp"
#include "util/bytes.hpp"
#include "util/compress.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <sstream>

namespace n = fire::network;
namespace u = fire::util;
namespace sc = fire::security;

namespace fire
{
    namespace message
    {

        namespace
        {
            const double MIN_THREAD_SLEEP = 1; //in milliseconds 
            const double MAX_THREAD_SLEEP = 51; //in milliseconds 
            const double SLEEP_STEP = 5;
            const double QUIT_SLEEP = 500;
            const size_t POOL_SIZE = 30; //small pool size for now
        }

        metadata::encryption_type to_message_encryption_type(sc::encryption_type s)
        {
            metadata::encryption_type r;
            switch(s)
            {
                case sc::encryption_type::plaintext: r = metadata::encryption_type::plaintext; break;
                case sc::encryption_type::symmetric: r = metadata::encryption_type::symmetric; break;
                case sc::encryption_type::asymmetric: r = metadata::encryption_type::asymmetric; break;
                default: CHECK(false && "missed case");
            }
            return r;
        }

        void in_thread(master_post_office* o)
        try
        {
            REQUIRE(o);
            REQUIRE(o->_encrypted_channels);

            double thread_sleep = MIN_THREAD_SLEEP;

            n::endpoint ep;
            while(!o->_done)
            try
            {
                //get data from outside world
                u::bytes data;
                if(!o->_connections.receive(ep, data))
                {
                    u::sleep_thread(thread_sleep);
                    thread_sleep = std::min(MAX_THREAD_SLEEP, thread_sleep + SLEEP_STEP);
                    continue;
                }
                thread_sleep = MIN_THREAD_SLEEP;

                if(o->_outside_stats.on) o->_outside_stats.in_push_count++;

                //construct address as conversation id and decrypt message
                auto sid = n::make_address_str(ep);

                sc::encryption_type et;
                data = o->_encrypted_channels->decrypt(sid, data, et);

                //could not decrypt, skip
                if(data.empty()) continue;

                //uncompress decrypted data
                data = u::uncompress(data);

                //unable to decompress, skip
                if(data.empty()) continue;

                //parse message
                message m;
                u::decode(data, m);

                //skip bad message
                if(m.meta.to.empty()) continue;

                //insert the from_ip, from_port and other metadata
                m.meta.extra["from_protocol"] = ep.protocol;
                m.meta.extra["from_ip"] = ep.address;
                m.meta.extra["from_port"] = ep.port;
                m.meta.encryption = to_message_encryption_type(et);
                m.meta.source = metadata::remote;

                //pop off master address
                m.meta.to.pop_front();

                //send message to interal component
                o->send(m);
                if(o->_outside_stats.on) o->_outside_stats.in_pop_count++;
            }
            catch(std::exception& e)
            {
                LOG << "error recieving message from " << ep.address << ":" << ep.port << ". " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "error recieving message from " << ep.address << ":" << ep.port << ". unknown error." << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: master_post::in_thread" << std::endl;
        }

        void encrypt_message(
                u::bytes& data,
                const message& m, 
                const std::string& conversation_id,
                security::encrypted_channels& sl)
        {
            switch(m.meta.encryption)
            {
                case metadata::encryption_type::plaintext: 
                    {
                        data = sl.encrypt_plaintext(data);
                        break;
                    }
                case metadata::encryption_type::symmetric:
                    {
                        data = sl.encrypt_symmetric(conversation_id, data);
                        break;
                    }
                case metadata::encryption_type::asymmetric:
                    {
                        data = sl.encrypt_asymmetric(conversation_id, data);
                        break;
                    }
                case metadata::encryption_type::conversation: 
                    {
                        data = sl.encrypt(conversation_id, data);
                        break;
                    }
                default:
                    CHECK(false && "missed type");
            }
        }


        void out_thread(master_post_office* o)
        try
        {
            REQUIRE(o);

            std::string last_address;

            while(!o->_done)
            try
            {
                //get message from queue
                message m;
                if(!o->_out.pop(m, true))
                    continue;

                REQUIRE_GREATER_EQUAL(m.meta.from.size(), 1);
                REQUIRE_GREATER_EQUAL(m.meta.to.size(), 1);

                const std::string outside_queue_address = m.meta.to.front();
                last_address = outside_queue_address;

                //encode, compress, and encrypt message
                auto data = u::encode(m);
                data = u::compress(data);

                encrypt_message(
                        data, 
                        m, 
                        outside_queue_address,
                        *o->_encrypted_channels);

                //send message over wire
                o->_connections.send(outside_queue_address, data, m.meta.robust);

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
            u::sleep_thread(QUIT_SLEEP);
        }
        catch(...)
        {
            LOG << "exit: master_post::out_thread" << std::endl;
        }

        master_post_office::master_post_office(
                const std::string& in_host,
                n::port_type in_port,
                sc::encrypted_channels_ptr sl) : 
            _in_host(in_host),
            _in_port{in_port},
            _connections{POOL_SIZE, in_port, false},
            _encrypted_channels{sl}
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
            _out.done();
            _in_thread->join();
            _out_thread->join();
        }

        bool master_post_office::send_outside(const message& m)
        {
            if(_outside_stats.on) _outside_stats.out_push_count++;
            _out.push(m);
            return true;
        }

        const network::udp_stats& master_post_office::get_udp_stats() const
        {
            return _connections.get_udp_stats();
        }
    }
}
