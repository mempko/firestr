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

#include "network/connection_manager.hpp"

#include "util/dbc.hpp"
#include "util/log.hpp"

#include <boost/lexical_cast.hpp>

namespace u = fire::util;

namespace fire
{
    namespace network
    {
        void tcp_send_thread(connection_manager*);

        connection_manager::connection_manager(size_t size, port_type local_port) :
            _pool(size),
            _local_port{local_port},
            _next_available{0},
            _rstate{receive_state::IN_UDP1},
            _done{false}
        {
            teardown_and_repool_tcp_connections();
            create_udp_endpoint();

            _tcp_send_thread.reset(new std::thread{tcp_send_thread, this});

            ENSURE_FALSE(_pool.empty());
            ENSURE(_in);
            ENSURE(_udp_con);
            ENSURE(_tcp_send_thread);
            ENSURE_EQUAL(_next_available, 0);
        }

        connection_manager::~connection_manager()
        {
            _done = true;
            _tcp_send_queue.done();
            if(_tcp_send_thread) _tcp_send_thread->join();
        }

        void connection_manager::create_udp_endpoint()
        {
            //create listen socket
            asio_params udp_p = {
                asio_params::udp, 
                asio_params::bind, 
                "", //uri
                "", //host
                0, //port
                _local_port,
                false, //block;
                0, // wait;
                true, //track_incoming;
            };
            _udp_con = create_udp_queue(udp_p);
        }
        void connection_manager::create_tcp_endpoint()
        {
            REQUIRE_FALSE(_in);
            REQUIRE_GREATER(_local_port, 0);

            auto listen_address = make_tcp_address("*", _local_port);

            queue_options qo = { 
                {"bnd", "1"},
                {"block", "0"},
                {"track_incoming", "1"}};

            _in = create_tcp_queue(listen_address, qo);
            ENSURE(_in);
        }

        void connection_manager::create_tcp_pool()
        {
            REQUIRE(!_pool.empty());

            //create outgoing params
            asio_params p = {
                asio_params::tcp, 
                asio_params::delayed_connect, 
                "", //uri
                "", //host
                0, //port
                _local_port,
                false, //block;
                0, // wait;
                false, //track_incoming;
            };

            //create socket pool
            for(size_t i = 0; i < _pool.size(); ++i)
                _pool[i] = std::make_shared<tcp_queue>(p);
        }

        void connection_manager::teardown_and_repool_tcp_connections()
        {
            LOG << "creating listening tcp connection and outgoing pool..." << std::endl;
            _next_available = 0;
            _in.reset();
            _in_connections.clear();
            _out.clear();
            for(auto& pv : _pool) pv.reset();

#ifdef __APPLE__
            create_tcp_endpoint();
#endif
            create_tcp_pool();

#ifndef __APPLE__
            create_tcp_endpoint();
#endif
        }

        tcp_queue_ptr connection_manager::get_connected_queue(const std::string& address)
        {
            auto p = _out.find(address);
            if(p != _out.end()) 
            {
                auto& c = _pool[p->second];
                CHECK(c);
                if(c->is_connecting() || c->is_connected()) 
                    return c;
            }

            return {};
        }

        tcp_queue_ptr connection_manager::connect(const std::string& address)
        try
        {
            {
                u::mutex_scoped_lock l(_mutex);
                auto q = get_connected_queue(address);
                if(q) return q;
            }


            //if we ran out of connections in the out pool then first check if any 
            //are connected. If any are connected then the message is not sent and
            //we are screwed for now with the algorithm. Otherwise we must have lost
            //internet connection and need to tear down and recreate the out connection
            //pool
            if(_next_available >= _pool.size()) 
            {
                u::mutex_scoped_lock l(_mutex);
                bool any_connected = false;
                for(auto& c : _pool) 
                {
                    CHECK(c);
                    if(c->is_connected()) 
                        any_connected = true;
                }

                if(any_connected)
                {
                    LOG << "ran out of outgoing tcp connections..." << std::endl;
                    return {};
                } 
                teardown_and_repool_tcp_connections();
            }

            CHECK_RANGE(_next_available, 0, _pool.size());

            //parse address to host, port
            auto a = parse_address(address);

            //increment index in pool
            auto i = _next_available;
            _next_available++;

            //assign address to socket and connect
            u::mutex_scoped_lock l(_mutex);
            _out[address] = i;
            _pool[i]->connect(a.host, a.port);

            ENSURE_BETWEEN(_next_available, 0, _pool.size());
            return _pool[i];
        }
        catch(std::exception& e)
        {
            LOG << "error connecting to `" << address << "'. " << e.what() << std::endl; 
        }
        catch(...)
        {
            LOG << "unknown error connecting to `" << address << "'." << std::endl; 
        }

        bool connection_manager::send(const std::string& to, const u::bytes& b)
        try
        {
            INVARIANT(_udp_con);

            auto type = determine_type(to);

            //if tcp, then push it to the tcp send queue
            //which is processed by another thread so that
            //udp connections are not blocked by tcp
            if(type == asio_params::tcp)
            {
                _tcp_send_queue.push({to, b});
                return true;
            }

            CHECK(type == asio_params::udp);

            auto a = parse_address(to);
            endpoint ep { UDP, a.host, a.port};
            endpoint_message em{ep, b}; 
            return _udp_con->send(em);
        }
        catch(std::exception& e)
        {
            LOG << "error sending message to `" << to << "' (" << b.size() << " bytes). " << e.what() << std::endl; 
        }
        catch(...)
        {
            LOG << "unknown error sending message to `" << to << "' (" << b.size() << " bytes)." << std::endl; 
        }

        void connection_manager::transition_udp_state()
        {
            REQUIRE(_rstate != IN_TCP);
            REQUIRE(_rstate != OUT_TCP);

            auto ps = _rstate; 
            switch(_rstate)
            {
                case receive_state::IN_UDP1: _rstate = receive_state::IN_UDP2; break;
                case receive_state::IN_UDP2: _rstate = receive_state::IN_UDP3; break;
                case receive_state::IN_UDP3: _rstate = receive_state::IN_UDP4; break;
                case receive_state::IN_UDP4: _rstate = receive_state::IN_UDP5; break;
                case receive_state::IN_UDP5: _rstate = receive_state::IN_UDP6; break;
                case receive_state::IN_UDP6: _rstate = receive_state::IN_UDP7; break;
                case receive_state::IN_UDP7: _rstate = receive_state::IN_UDP8; break;
                case receive_state::IN_UDP8: _rstate = receive_state::IN_TCP; break;
                default: CHECK(false && "missed case");
            }

            ENSURE(ps != receive_state::IN_UDP8 || _rstate == receive_state::IN_TCP) 
            ENSURE(_rstate != ps);
        }

        bool connection_manager::receive(endpoint& ep, u::bytes& b)
        {

            //if we quite in prior call and got to done state
            //or returned with a message in OUT_TCP case, we
            //reset to IN_UPD and start over
            if(_rstate == receive_state::DONE) 
                _rstate = receive_state::IN_UDP1;

            while(_rstate != receive_state::DONE)
            {
                switch(_rstate)
                {
                    case receive_state::IN_UDP1:
                    case receive_state::IN_UDP2:
                    case receive_state::IN_UDP3:
                    case receive_state::IN_UDP4:
                    case receive_state::IN_UDP5:
                    case receive_state::IN_UDP6:
                    case receive_state::IN_UDP7:
                    case receive_state::IN_UDP8:
                        {
                            endpoint_message um;
                            if(_udp_con->receive(um))
                            {
                                transition_udp_state();

                                ep = um.ep;
                                b = std::move(um.data);
                                return true;
                            }
                            _rstate = receive_state::IN_TCP;
                        }
                        break;

                    case receive_state::IN_TCP:
                        {
                            _rstate = receive_state::OUT_TCP;
                            u::mutex_scoped_lock l(_mutex);
                            INVARIANT(_in);
                            if(_in->receive(b)) 
                            {
                                auto s = _in->get_socket();
                                CHECK(s);
                                ep = s->get_endpoint();
                                _in_connections[make_address_str(ep)] = s;
                                return true;
                            }
                        }
                        break;

                    case receive_state::OUT_TCP:
                        {
                            _rstate = receive_state::DONE;
                            u::mutex_scoped_lock l(_mutex);
                            for(auto p : _out)
                            {
                                auto i = p.second;
                                CHECK_RANGE(i, 0, _pool.size());

                                auto c = _pool[i];
                                CHECK(c);
                                if(c->receive(b)) 
                                {
                                    auto s = c->get_socket();
                                    CHECK(s);
                                    ep = s->get_endpoint();
                                    return true;
                                }
                            }
                        }
                        break;
                    default: CHECK(false && "missed case");
                }
            }

            ENSURE(_rstate == receive_state::DONE);
            return false;
        }

        bool connection_manager::is_disconnected(const std::string& addr)
        {
            u::mutex_scoped_lock l(_mutex);
            auto si = _in_connections.find(addr);
            if(si == _in_connections.end()) return true;
            CHECK(si->second);
            return si->second->is_disconnected();
        }

        void tcp_send_thread(connection_manager* m)
        {
            REQUIRE(m);
            while(!m->_done)
            try
            {
                send_item i;
                if(!m->_tcp_send_queue.pop(i, true))
                    continue;

                //first check in tcp_connections for matching address and use that to send
                //otherwise use outgoing tcp_connection.
                {
                    u::mutex_scoped_lock l(m->_mutex);

                    auto inp = m->_in_connections.find(i.to);
                    if(inp != m->_in_connections.end())
                    {
                        auto o = inp->second;
                        CHECK(o);
                        if(!o->is_disconnected()) 
                        {
                            o->send(i.data);
                            continue;
                        }
                    }
                }

                auto o = m->connect(i.to);
                if(o) o->send(i.data);
            }
            catch(std::exception& e)
            {
                LOG << "connection_manager: error sending tcp message. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "connection_manager: error sending tcp message. Unknown reason." << std::endl;
            }
        }
    }
}
