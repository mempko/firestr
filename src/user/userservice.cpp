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

#include "user/userservice.hpp"
#include "messages/greeter.hpp"
#include "messages/pinhole.hpp"
#include "util/uuid.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"

#include <stdexcept>
#include <ctime>
#include <thread>
#include <random>

namespace m = fire::message;
namespace ms = fire::messages;
namespace n = fire::network;
namespace u = fire::util;
namespace us = fire::user;
namespace s = fire::service;

namespace fire
{
    namespace user
    {
        namespace
        {
            const std::string SERVICE_ADDRESS = "user_service";
            const std::string ADD_REQUEST = "add_user_request";
            const std::string REQUEST_CONFIRMED = "add_user_confirmed";
            const std::string REQUEST_REJECTED = "add_user_rejected";
            const std::string PING_REQUEST = "ping_request";
            const std::string PING = "!";
            const size_t QUIT_SLEEP = 500; //in milliseconds
            const size_t PING_THREAD_SLEEP = 500; //half a second
            const size_t PING_TICKS = 6; //3 seconds
            const size_t PING_THRESH = 3*PING_TICKS; 
            const char CONNECTED = 'c';
            const char DISCONNECTED = 'd';
            const size_t GREET_THREAD_SLEEP = 200; 
            const size_t STUN_WAIT_THRESH = 25;
            const size_t MIN_PORT = 55000;
            const size_t MAX_PORT = 65535;
        }

        struct ping 
        {
            std::string to;
            std::string from_id;
            char state;
        };

        m::message convert(const ping& r)
        {
            m::message m;
            m.meta.type = PING;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["from_id"] = r.from_id;
            m.data.resize(1);
            m.data[0] = r.state;

            return m;
        }

        void convert(const m::message& m, ping& r)
        {
            REQUIRE_EQUAL(m.meta.type, PING);
            REQUIRE_GREATER(m.meta.from.size(), 1);
            r.from_id = m.meta.extra["from_id"].as_string();
            r.state = m.data.size() == 1 ? m.data[0] : DISCONNECTED;
        }

        struct ping_request 
        {
            std::string to;
            std::string from_id;
            std::string from_ip;
            std::string from_port;
            int send_back;
        };

        m::message convert(const ping_request& r)
        {
            m::message m;
            m.meta.type = PING_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["from_id"] = r.from_id;
            m.meta.extra["from_port"] = r.from_port;
            m.meta.extra["send_back"] = r.send_back;

            return m;
        }

        void convert(const m::message& m, ping_request& r)
        {
            REQUIRE_EQUAL(m.meta.type, PING_REQUEST);
            REQUIRE_GREATER(m.meta.from.size(), 1);
            r.from_id = m.meta.extra["from_id"].as_string();
            r.from_ip = m.meta.extra["from_ip"].as_string();
            r.from_port = m.meta.extra["from_port"].as_string();
            r.send_back = m.meta.extra["send_back"];
        }

        struct req_rejected 
        {
            std::string address;
            std::string key;
        };

        struct req_confirmed
        {
            std::string to;
            std::string key;
            user_info_ptr from;
        };

        m::message convert(const add_request& r)
        {
            REQUIRE(r.from);

            m::message m;
            m.meta.type = ADD_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["key"] = r.key;
            m.data = u::encode(*r.from);

            return m;
        }

        void convert(const m::message& m, add_request& r)
        {
            REQUIRE_EQUAL(m.meta.type, ADD_REQUEST);
            REQUIRE_GREATER(m.meta.from.size(), 1);

            std::string from = m.meta.from.front();

            r.from.reset(new user_info);
            u::decode(m.data, *r.from);
            r.from->address(from);
            r.to = "";
            r.key = m.meta.extra["key"].as_string();

            ENSURE(r.from);
        }

        m::message convert(const req_confirmed& r)
        {
            REQUIRE(r.from);

            m::message m;
            m.meta.type = REQUEST_CONFIRMED;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["key"] = r.key;
            m.data = u::encode(*r.from);

            return m;
        }

        void convert(const m::message& m, req_confirmed& r)
        {
            REQUIRE_EQUAL(m.meta.type, REQUEST_CONFIRMED);
            REQUIRE_GREATER(m.meta.from.size(), 1);

            std::string from = m.meta.from.front();

            r.from.reset(new user_info);
            u::decode(m.data, *r.from);
            r.from->address(from);
            r.to = "";
            r.key = m.meta.extra["key"].as_string();

            ENSURE(r.from);
        }

        m::message convert(const req_rejected& r)
        {
            m::message m;
            m.meta.type = REQUEST_REJECTED;
            m.meta.extra["key"] = r.key;
            m.meta.to = {r.address, SERVICE_ADDRESS}; 

            return m;
        }

        void convert(const m::message& m, req_rejected& r)
        {
            REQUIRE_EQUAL(m.meta.type, REQUEST_REJECTED);
            REQUIRE_GREATER(m.meta.from.size(), 1);

            r.key = m.meta.extra["key"].as_string();
            r.address = m.meta.from.front();
        }

        user_service::user_service(user_service_context& c) :
            s::service{SERVICE_ADDRESS, c.events},
            _home{c.home},
            _in_host{c.host},
            _in_port{c.port},
            _stun_server{c.stun_server},
            _stun_port{c.stun_port},
            _stun{c.stun},
            _greeter_server{c.greeter_server},
            _greeter_port{c.greeter_port},
            _done{false}
        {
            REQUIRE_FALSE(c.home.empty());

            _user = load_user(_home); 
            if(!_user) throw std::runtime_error{"no user found at `" + _home + "'"};
            update_address(n::make_bst_address(c.host, c.port));

            init_greet();
            init_ping();

            INVARIANT(_user);
            INVARIANT(mail());
            ENSURE(_ping_thread);
        }

        user_service::~user_service()
        {
            INVARIANT(_ping_thread);
            send_ping(DISCONNECTED);
            u::sleep_thread(QUIT_SLEEP);

            _done = true;
            _ping_thread->join();
            _greet_thread->join();
        }

        void user_service::update_address(const std::string& address)
        {
            INVARIANT(_user);
            if(_user->info().address() == address) return;

            _user->info().address(address);
            save_user(_home, *_user);
        }

        void ping_thread(user_service* s);
        void user_service::init_ping()
        {
            REQUIRE(!_ping_thread);

            _ping_thread.reset(new std::thread{ping_thread, this});

            ENSURE(_ping_thread);
        }

        void greet_thread(user_service* s);
        void user_service::init_greet()
        {
            REQUIRE(!_greet_thread);
            REQUIRE(!_greet_queue);

            //make connection to greeter server if 
            //we setup the params
            if(!_greeter_server.empty() && !_greeter_port.empty())
            {
                auto address = n::make_bst_address(_greeter_server, _greeter_port);
                n::queue_options qo = { 
                    {"con", "1"},
                    {"block", "0"}};
                _greet_queue = n::create_message_queue(address, qo);
            }

            _state = started_greet;
            _greet_thread.reset(new std::thread{greet_thread, this});

            ENSURE(_greet_thread);
        }

        void make_pinhole(const std::string& ip, const std::string remote_port, const std::string& local_port)
        {
            auto a = n::make_bst_address(ip, remote_port, local_port);
            std::cout << "making pinhole: " << a << std::endl;
            n::queue_options qo = { 
                {"con", "1"},
                {"block", "0"},
                {"local_port", local_port}};

            auto pin_hole = n::create_message_queue(a, qo);
            CHECK(pin_hole);

            ms::pinhole pm;
            m::message m = pm;
            pin_hole->send(u::encode(m));
        }

        bool available(size_t ticks) { return ticks <= PING_THRESH; }

        void user_service::message_recieved(const message::message& m)
        {
            if(m.meta.type == PING)
            {
                u::mutex_scoped_lock l(_ping_mutex);
                ping r;
                convert(m, r);

                auto p = _last_ping.find(r.from_id);
                if(p == _last_ping.end()) return;

                auto& ticks = p->second;
                bool prev_state = available(ticks);

                if(r.state == CONNECTED)
                {
                    ticks = 0;
                }
                else
                {
                    ticks = PING_THRESH + PING_THRESH;
                    CHECK_FALSE(available(ticks));
                }

                bool cur_state = available(ticks);
                if(cur_state != prev_state)
                {
                    if(cur_state) fire_contact_connected_event(r.from_id);
                    else fire_contact_disconnected_event(r.from_id);
                }
            }
            else if(m.meta.type == PING_REQUEST)
            {
                ping_request r;
                convert(m, r);

                auto c = _user->contacts().by_id(r.from_id);
                if(!c) return;

                std::cerr << "got ping request from: " << c->address() << " updated: " << r.from_ip << ":" << r.from_port << std::endl;

                //update contact address to the one specified
                //if it is different.
                update_contact_address(c->id(), r.from_ip, r.from_port);

                //we are already connected to this contact
                //send ping and return
                if(contact_available(c->id())) 
                {
                    if(r.send_back) send_ping_request(c, false);
                    return;
                }

                fire_contact_connected_event(c->id());
                send_ping_request(c);
            }
            else if(m.meta.type == ADD_REQUEST)
            {
                add_request r;
                convert(m, r);

                CHECK(r.from);

                //if contact already exists send confirmation
                //otherwise add to pending requests

                auto f = _user->contacts().by_id(r.from->id());
                if(f) send_confirmation(f->id(), r.key);
                else 
                {
                    u::mutex_scoped_lock l(_mutex);
                    _pending_requests[r.from->id()] = r;

                    fire_new_contact_event(r.from->id());
                }
            }
            else if(m.meta.type == REQUEST_CONFIRMED)
            {
                req_confirmed r;
                convert(m, r);

                CHECK(r.from);

                if(_sent_requests.count(r.key))
                {
                    _sent_requests.erase(r.key);
                    confirm_contact(r.from);
                }
            }
            else if(m.meta.type == REQUEST_REJECTED)
            {
                req_rejected r;
                convert(m, r);
                _sent_requests.erase(r.key);
            }
            else if(m.meta.type == ms::GREET_FIND_RESPONSE)
            {
                if(!_greet_queue) return;

                ms::greet_find_response rs{m};
                if(!rs.found()) return;

                std::cerr << "got greet response: " << rs.ip() << ":" << rs.port() << std::endl;

                //update address info
                update_contact_address(rs.id(), rs.ip(), rs.port());

                //make pinhole
                if(_stun && !rs.from_port().empty())
                    make_pinhole(rs.ip(), rs.from_port(), _in_port);

                //send ping request using new address
                auto c = _user->contacts().by_id(rs.id());
                if(c) send_ping_request(c);
            }
            else
            {
                throw std::runtime_error{"unsuported message type `" + m.meta.type +"'"};
            }
        }

        void user_service::update_contact_address(const std::string& id, const std::string& ip, const std::string& port)
        {
            INVARIANT(_user);
            u::mutex_scoped_lock l(_mutex);

            auto c = _user->contacts().by_id(id);
            if(!c) return;

            std::string local_port;
            {
                u::mutex_scoped_lock l(_state_mutex);
                local_port = _local_ports[c->id()];
            }
            auto a = n::make_bst_address(ip, port, local_port);

            if(c->address() == a) return;

            std::cerr << "updating address from: " << c->address() << " to: " << a << std::endl;
            c->address(a);
            save_user(_home, *_user);
        }

        local_user& user_service::user()
        {
            INVARIANT(_user);
            return *_user;
        }

        const local_user& user_service::user() const
        {
            INVARIANT(_user);
            return *_user;
        }

        const std::string& user_service::home() const
        {
            ENSURE_FALSE(_home.empty());
            return _home;
        }

        void user_service::confirm_contact(user_info_ptr contact)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(mail());
            REQUIRE(contact);

            //add user
            _user->contacts().add(contact);
            save_user(_home, *_user);
        }

        const add_requests& user_service::pending_requests() const
        {
            return _pending_requests;
        }

        void user_service::attempt_to_add_contact(const std::string& address)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(mail());

            std::string ex = m::external_address(address);
            std::string key = u::uuid();

            user_info_ptr self{new user_info{_user->info()}};
            add_request r{ex, key, self};

            _sent_requests.insert(key);
            mail()->push_outbox(convert(r));
        }

        void user_service::send_confirmation(const std::string& id, std::string key)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(mail());

            if(key.empty())
            {
                auto p = _pending_requests.find(id);
                CHECK(p != _pending_requests.end());
                CHECK(p->second.from);

                _user->contacts().add(p->second.from);
                save_user(_home, *_user);

                key = p->second.key;
            }

            user_info_ptr user = _user->contacts().by_id(id);
            CHECK(user);

            user_info_ptr self{new user_info{_user->info()}};
            req_confirmed r{user->address(), key, self};

            mail()->push_outbox(convert(r));
            _pending_requests.erase(id);
        }

        void user_service::send_rejection(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(mail());

            //get user who wanted to be added
            auto p = _pending_requests.find(id);
            CHECK(p != _pending_requests.end());
            auto user = p->second.from;
            auto key = p->second.key;

            //remove request
            if(p != _pending_requests.end())
                _pending_requests.erase(id);

            req_rejected r{user->address(), key};
            mail()->push_outbox(convert(r));
        }

        void ping_thread(user_service* s)
        try
        {
            size_t send_ticks = 0;
            REQUIRE(s);
            REQUIRE(s->_user);
            while(!s->_done)
            try
            {
                if(send_ticks > PING_TICKS)
                {
                    s->send_ping(CONNECTED);
                    send_ticks = 0;
                } 
                else send_ticks++; 

                {
                    u::mutex_scoped_lock l(s->_ping_mutex);
                    for(auto& p : s->_last_ping)
                    {
                        auto& ticks = p.second;
                        bool prev_state = available(ticks);
                        ticks++;
                        bool cur_state = available(ticks);

                        //if we changed state, fire event
                        if(cur_state != prev_state)
                        {
                            CHECK(!cur_state); // should only flip one way
                            s->fire_contact_disconnected_event(p.first);
                        }
                    }
                }
                u::sleep_thread(PING_THREAD_SLEEP);
            }
            catch(std::exception& e)
            {
                std::cerr << "Error in ping thread: " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "Unexpected error in ping thread." << std::endl;
            }
        }
        catch(...)
        {
            std::cerr << "exit: user_service::ping_thread" << std::endl;
        }

        std::string random_port(size_t seed)
        {
            std::uniform_int_distribution<size_t> distribution(MIN_PORT, MAX_PORT);
            std::mt19937 engine; 
            engine.seed(seed + std::time(0));
            auto generate = std::bind(distribution, engine);
            return boost::lexical_cast<std::string>(generate());
        }

        void greet_thread(user_service* s)
        try
        {
            size_t send_ticks = 0;
            REQUIRE(s);
            REQUIRE(s->_user);
            REQUIRE(s->_state == user_service::started_greet);

            size_t stun_wait = 0;
            size_t tries_left = 10;

            while(!s->_done && tries_left > 0 && s->_state != user_service::done_greet)
            try
            {
                if(s->_state == user_service::started_greet)
                {
                    if(s->_stun)
                    {
                        CHECK_FALSE(s->_stun_port.empty());
                        CHECK_FALSE(s->_in_port.empty());
                        
                        std::cerr << "sending stun request to " << s->_stun_server << ":" << s->_stun_port << std::endl;
                        s->_stun->send_stun_request();

                        u::mutex_scoped_lock l(s->_state_mutex);
                        s->_state = user_service::sent_stun;
                        stun_wait = 0;
                    }
                    else
                    {
                        u::mutex_scoped_lock l(s->_state_mutex);
                        s->_state = user_service::got_stun;
                    }
                }
                else if(s->_state == user_service::sent_stun)
                {
                    if(s->_stun->state() == n::stun_success)
                    {
                        auto address = n::make_bst_address(s->_stun->external_ip(), s->_stun->external_port());
                        std::cerr << "got stun address " << address << std::endl;
                        s->update_address(address);

                        {
                            u::mutex_scoped_lock l(s->_state_mutex);
                            s->_state = user_service::got_stun;
                        }
                    }
                    else if(s->_stun->state() == n::stun_failed)
                    {
                        //stun failed, quit
                        stun_wait = STUN_WAIT_THRESH;
                    }
                    else
                    {
                        stun_wait++;
                    }

                    if(stun_wait >= STUN_WAIT_THRESH)
                    {
                        u::mutex_scoped_lock l(s->_state_mutex);
                        s->_state = user_service::failed_greet;
                    }
                }
                else if(s->_state == user_service::got_stun)
                {
                    if(s->_greet_queue)
                    {
                        auto return_port = random_port(boost::lexical_cast<size_t>(s->_in_port));
                        ms::greet_register r
                        {
                            s->_user->info().id(), 
                            (s->_stun ? s->_stun->external_ip() : ""), //if no stun, let greeter use socket ip
                            (s->_stun ? s->_stun->external_port() : s->_in_port),
                            return_port,
                            SERVICE_ADDRESS
                        };

                        m::message m = r;
                        s->_greet_queue->send(u::encode(m));
                
                        if(s->_stun)
                            make_pinhole(s->_greeter_server, return_port, s->_in_port);

                        u::mutex_scoped_lock l(s->_state_mutex);
                        s->_state = user_service::sent_greet;
                    }
                    else
                    {
                        u::mutex_scoped_lock l(s->_state_mutex);
                        s->_state = user_service::done_greet;
                    }
                }
                else if(s->_state == user_service::sent_greet)
                {
                    CHECK(s->_greet_queue);

                    for(auto c : s->_user->contacts().list())
                    {
                        CHECK(c);

                        std::string local_port;

                        //if we are connecting outside nat, assign the port that
                        //the contact should bind to when connecting to this user.
                        if(s->_stun)
                        {
                            u::mutex_scoped_lock l(s->_state_mutex);
                            local_port = random_port(boost::lexical_cast<size_t>(s->_in_port));
                            s->_local_ports[c->id()] = local_port;
                        }

                        ms::greet_find_request r {s->_user->info().id(), c->id(), local_port};

                        //send request
                        m::message m = r;
                        s->_greet_queue->send(u::encode(m));

                    }

                    u::mutex_scoped_lock l(s->_state_mutex);
                    s->_state = user_service::sent_contact_query;
                }
                else if(s->_state == user_service::sent_contact_query)
                {
                    u::mutex_scoped_lock l(s->_state_mutex);
                    s->_state = user_service::done_greet;
                }
                else if(s->_state == user_service::failed_greet)
                {
                    u::mutex_scoped_lock l(s->_state_mutex);
                    tries_left--;
                    s->_state = user_service::started_greet;
                }
                else CHECK(false && "missed state");

                u::sleep_thread(GREET_THREAD_SLEEP);
            }
            catch(std::exception& e)
            {
                u::mutex_scoped_lock l(s->_state_mutex);
                s->_state = user_service::failed_greet;
                std::cerr << "Error in greet thread: " << e.what() << std::endl;
            }
            catch(...)
            {
                u::mutex_scoped_lock l(s->_state_mutex);
                s->_state = user_service::failed_greet;
                std::cerr << "Unexpected error in greet thread." << std::endl;
            }

            std::cerr << "done greet..." << std::endl;
            u::mutex_scoped_lock l(s->_state_mutex);
            s->_state = user_service::done_greet;

            //now we are done, send the ping port requests
            if(!s->_greet_queue) s->send_ping_requests();
        }
        catch(std::exception& e)
        {
            std::cerr << "exit: user_service::greet_thread: " << e.what() << std::endl;
        }
        catch(...)
        {
            std::cerr << "exit: user_service::greet_thread" << std::endl;
        }

        bool user_service::contact_available(const std::string& id) const
        {
            u::mutex_scoped_lock l(_ping_mutex);
            return _connected.count(id) > 0;
        }

        void user_service::send_ping(char s)
        {
            REQUIRE(s == CONNECTED || s == DISCONNECTED);
            u::mutex_scoped_lock l(_ping_mutex);


            //send pint message to all connected contacts
            for(auto p : _connected)
            {
                auto c = p.second;
                CHECK(c);

                ping r = {r.to = c->address(), _user->info().id(), s};
                mail()->push_outbox(convert(r));
            }
        }

        void user_service::send_ping_requests()
        {
            INVARIANT(_user);

            for(auto c : _user->contacts().list())
                send_ping_request(c);
        }

        void user_service::send_ping_request(us::user_info_ptr c, bool send_back)
        {
            INVARIANT(_user);
            INVARIANT(mail());

            ping_request a
            {
                c->address(), 
                _user->info().id(), 
                (_stun ? _stun->external_ip() : _in_host), 
                (_stun ? _stun->external_port() : _in_port),
                send_back
            };
            mail()->push_outbox(convert(a));

        }

        void user_service::fire_new_contact_event(const std::string& id)
        {
            event::new_contact e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_connected_event(const std::string& id)
        {
            auto c = _user->contacts().by_id(id);
            if(!c) return;

            _connected[c->id()] = c;
            _last_ping[c->id()] = 0;

            event::contact_connected e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_disconnected_event(const std::string& id)
        {
            _connected.erase(id);
            _last_ping.erase(id);

            event::contact_disconnected e{id};
            send_event(event::convert(e));
        }

        namespace event
        {
            const std::string NEW_CONTACT = "new_contact";
            const std::string CONTACT_CONNECTED = "contact_con";
            const std::string CONTACT_DISCONNECTED = "contact_discon";

            m::message convert(const new_contact& c)
            {
                m::message m;
                m.meta.type = NEW_CONTACT;
                m.data = u::to_bytes(c.id);
                return m;
            }

            void convert(const m::message& m, new_contact& c)
            {
                REQUIRE_EQUAL(m.meta.type, NEW_CONTACT);
                c.id = u::to_str(m.data);
            }

            m::message convert(const contact_connected& c)
            {
                m::message m;
                m.meta.type = CONTACT_CONNECTED;
                m.data = u::to_bytes(c.id);
                return m;
            }

            void convert(const m::message& m, contact_connected& c)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_CONNECTED);
                c.id = u::to_str(m.data);
            }

            m::message convert(const contact_disconnected& c)
            {
                m::message m;
                m.meta.type = CONTACT_DISCONNECTED;
                m.data = u::to_bytes(c.id);
                return m;
            }

            void convert(const m::message& m, contact_disconnected& c)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_DISCONNECTED);
                c.id = u::to_str(m.data);
            }
        }
    }
}
