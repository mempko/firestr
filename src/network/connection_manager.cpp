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

#include "network/connection_manager.hpp"

#include "util/dbc.hpp"

#include <boost/lexical_cast.hpp>

namespace u = fire::util;

namespace fire
{
    namespace network
    {
        connection_manager::connection_manager(size_t size, const std::string& local_port) :
            _pool(size),
            _local_port{local_port},
            _next_available{0}
        {
            //create outgoing params
            asio_params p = {
                asio_params::delayed_connect, 
                "", //uri
                "", //host
                "", //port
                _local_port,
                false, //block;
                0, // wait;
                false, //track_incoming;
            };

            //create socket pool
            for(size_t i = 0; i < size; ++i)
                _pool[i].reset(new boost_asio_queue{p});

            //create listen socket
            auto listen_address = make_tcp_address("*", _local_port);

            queue_options qo = { 
                {"bnd", "1"},
                {"block", "0"},
                {"track_incoming", "1"}};

            _in = create_message_queue(listen_address, qo);

            INVARIANT(_in);
        }

        boost_asio_queue_ptr connection_manager::connect(const std::string& address)
        {
            u::mutex_scoped_lock l(_mutex);
            REQUIRE_RANGE(_next_available, 0, _pool.size());

            //return if we are already assigned a socket to that address
            auto p = _out.find(address);
            if(p != _out.end()) return _pool[p->second];

            //parse address to host, port
            auto a = parse_address(address);

            auto i = _next_available;
            _next_available++;

            //assign address to socket and connect
            _out[address] = i;
            _pool[i]->connect(a.host, a.port);

            ENSURE_BETWEEN(_next_available, 0, _pool.size());
            return _pool[i];
        }

        boost_asio_queue_ptr connection_manager::get(const std::string& address)
        {
            u::mutex_scoped_lock l(_mutex);
            auto p = _out.find(address);
            if(p == _out.end()) return {};

            auto i = p->second;
            CHECK_RANGE(i, 0, _pool.size());

            auto q = _pool[i];
            ENSURE(q);
            return q;
        }

        bool connection_manager::recieve(u::bytes& b)
        {
            INVARIANT(_in);
            if(_in->recieve(b)) 
            {
                _last_recieved.push(_in->get_socket());
                return true;
            }

            for(auto p : _out)
            {
                auto i = p.second;
                CHECK_RANGE(i, 0, _pool.size());

                auto c = _pool[i];
                CHECK(c);
                if(c->recieve(b)) 
                {
                    _last_recieved.push(c->get_socket());
                    return true;
                }
            }

            return false;
        }

        connection* connection_manager::get_socket()
        {
            INVARIANT(_in);
            return _last_recieved.pop();
        }
    }
}
