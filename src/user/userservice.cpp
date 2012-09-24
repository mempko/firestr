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
#include "util/uuid.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"

#include <stdexcept>
#include <thread>

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
            const size_t QUIT_SLEEP = 500; //in milliseconds
            const size_t PING_THREAD_SLEEP = 500; //half a second
            const size_t PING_TICKS = 6; //3 seconds
            const size_t PING_THRESH = 3*PING_TICKS; 
            const char CONNECTED = 'c';
            const char DISCONNECTED = 'd';
            const size_t GREET_THREAD_SLEEP = 200; 
            const size_t STUN_WAIT_THRESH = 25;
        }

        struct ping_request 
        {
            std::string to;
            std::string from_id;
            std::string from_address;
            std::string port;
            int send_back;
        };

        m::message convert(const ping_request& r)
        {
            m::message m;
            m.meta.type = PING_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["from_id"] = r.from_id;
            m.meta.extra["from_address"] = r.from_address;
            m.meta.extra["send_back"] = r.send_back;
            m.data = u::to_bytes(r.port);

            return m;
        }

        void convert(const m::message& m, ping_request& r)
        {
            REQUIRE_EQUAL(m.meta.type, PING_REQUEST);
            REQUIRE_GREATER(m.meta.from.size(), 1);
            r.from_id = m.meta.extra["from_id"].as_string();
            r.from_address = m.meta.extra["from_address"].as_string();
            r.send_back = m.meta.extra["send_back"];
            r.port = u::to_str(m.data);
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
            _ping_port{c.ping_port},
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
            update_address(n::make_zmq_address(c.host, c.port));

            init_ping();
            init_greet();

            INVARIANT(_user);
            INVARIANT(mail());
            ENSURE(_ping_queue);
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
            REQUIRE(!_ping_queue);

            auto ping_address = n::make_zmq_address("*", _ping_port);
            n::queue_options qo = { 
                {"pub", "1"}, 
                {"bnd", "1"},
                {"block", "0"}};
            _ping_queue = n::create_message_queue(ping_address, qo);
            _ping_thread.reset(new std::thread{ping_thread, this});

            ENSURE(_ping_queue);
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
                auto address = n::make_zmq_address(_greeter_server, _greeter_port);
                n::queue_options qo = { 
                    {"req", "1"}, 
                    {"con", "1"},
                    {"block", "1"}};
                _greet_queue = n::create_message_queue(address, qo);
            }

            _state = started_greet;
            _greet_thread.reset(new std::thread{greet_thread, this});

            ENSURE(_greet_thread);
        }

        std::string rewrite_port(const std::string& a, const std::string& port)
        {
            auto s = u::split<u::string_vect>(a, ":");
            if(s.size() != 3) return a;
            auto r = s[0] + ":" +  s[1] + ":" + port;
            return r;
        }

        bool construct_ping_address(
                std::string& address, 
                const us::user_info_ptr c, 
                const ping_request& r)
        {
            REQUIRE(c);

            //example address: zmq,tcp://host:port
            auto a = n::parse_address(c->address());
            auto s = u::split<u::string_vect>(a.location, ":");
            if(s.size() != 3) return false;

            const std::string host = s[0] + ":" + s[1];
            address = "zmq," + host + ":" + r.port;

            std::cerr << "creating ping connection to: " << address << std::endl;
            ENSURE_FALSE(address.empty());
            return true;
        }

        void user_service::message_recieved(const message::message& m)
        {
            if(m.meta.type == PING_REQUEST)
            {
                ping_request r;
                convert(m, r);

                auto c = _user->contacts().by_id(r.from_id);
                if(!c) return;

                std::cerr << "got ping request from: " << c->address() << " updated: " << r.from_address << std::endl;

                //update contact address to the one specified
                //if it is different.
                if(c->address() != r.from_address)
                {
                    update_contact_address(c->id(), r.from_address);
                    _ping_connection.erase(c->id());
                }
                //if the address is the same
                //check to see if we opened a connection to listen to their ping
                else
                {
                    u::mutex_scoped_lock l(_ping_mutex);
                    if(_ping_connection.count(c->id())) 
                    {
                        std::cerr << "already have connecton to: " << c->address() << " pinging back" << std::endl;
                        if(r.send_back) send_ping_address(c, false);
                        return;
                    }
                }

                std::cerr << "making new connection to: " << c->address() << std::endl;

                //otherwise we have a new connection
                std::string ping_address; 
                if(!construct_ping_address(ping_address, c, r)) return;

                init_ping_connection(c->id(), ping_address);
                send_ping_address(c);
                fire_contact_connected_event(c->id());
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
            else
            {
                throw std::runtime_error{"unsuported message type `" + m.meta.type +"'"};
            }

        }

        void user_service::update_contact_address(const std::string& id, const std::string& a)
        {
            INVARIANT(_user);
            u::mutex_scoped_lock l(_mutex);

            auto c = _user->contacts().by_id(id);
            if(!c) return;
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

        bool available(size_t ticks) { return ticks <= PING_THRESH; }

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
                    for(auto p : s->_ping_connection)
                    {
                        auto id = p.first;
                        auto queue = p.second;
                        auto& ticks = s->_last_ping[id];
                        bool prev_state = available(ticks);
                        bool cur_state = prev_state;

                        CHECK(queue);
                        u::bytes b;
                        if(queue->recieve(b) && b.size() == 1) 
                        {
                            char t = b[0]; 
                            if(t == CONNECTED)
                            {
                                ticks = 0;
                            }
                            else
                            {
                                ticks = PING_THRESH + PING_THRESH;
                                CHECK_FALSE(available(ticks));
                            }
                        }
                        else ticks++;

                        cur_state = available(ticks);

                        //if we changed state, fire event
                        if(cur_state != prev_state)
                        {
                            if(cur_state) s->fire_contact_connected_event(id);
                            else s->fire_contact_disconnected_event(id);
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

        void make_pinhole(const std::string& address, const std::string ext_port)
        {
            auto a = rewrite_port(address, ext_port);
            std::cout << "making pinhole: " << a << std::endl;
            n::queue_options qo = { 
                {"psh", "1"}, 
                {"con", "1"},
                {"wait", "50"},
                {"block", "1"}};
            auto pin_hole = n::create_message_queue(a, qo);
            CHECK(pin_hole);
            u::bytes b;
            b.push_back('a');
            b.push_back('b');
            b.push_back('c');
            pin_hole->send(b);
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
                        s->_state = user_service::done_greet;
                    }
                }
                else if(s->_state == user_service::sent_stun)
                {
                    if(s->_stun->state() == n::stun_success)
                    {
                        auto address = n::make_zmq_address(s->_stun->external_ip(), s->_stun->external_port());
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
                        if(s->_stun) 
                        {
                            auto a = n::make_zmq_address(s->_greeter_server, s->_greeter_port);
                            make_pinhole(a, s->_stun->external_port());
                        }
                        ms::greet_register r
                        {
                            s->_user->info().id(), 
                            s->_stun->external_ip(),
                            s->_stun->external_port() 
                        };

                        m::message m = r;
                        s->_greet_queue->send(u::encode(m));

                        u::bytes b;
                        s->_greet_queue->recieve(b);
                        u::decode(b, m);


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
                        ms::greet_find_request r {s->_user->info().id(), c->id(), SERVICE_ADDRESS};

                        //send request
                        m::message m = r;
                        s->_greet_queue->send(u::encode(m));

                        //get response
                        u::bytes response;
                        s->_greet_queue->recieve(response);
                        u::decode(response, m);

                        ms::greet_find_response rs{m};
                        if(!rs.found()) continue;

                        //update address info
                        auto new_address = n::make_zmq_address(rs.ip(), rs.port());
                        s->update_contact_address(rs.id(), new_address);
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
            s->send_ping_port_requests();
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
            auto tp = _last_ping.find(id);
            if(tp == _last_ping.end()) return false;

            return available(tp->second);
        }

        void user_service::send_ping(char t)
        {
            INVARIANT(_ping_queue);

            //either type of ping is connected, or disconnected
            REQUIRE(t == CONNECTED || t == DISCONNECTED);
            u::bytes b{t};
            _ping_queue->send(b);
        }

        void user_service::send_ping_port_requests()
        {
            INVARIANT(_user);

            for(auto c : _user->contacts().list())
                send_ping_address(c);
        }

        void user_service::init_ping_connection(const std::string& from_id, const std::string& ping_address)
        {
            u::mutex_scoped_lock l(_ping_mutex);

            n::queue_options qo = { 
                {"sub", "1"}, 
                {"con", "1"},
                {"block", "0"}};

            auto q = n::create_message_queue(ping_address, qo);
            CHECK(q);

            _ping_connection[from_id] = q;
            _last_ping[from_id] = 0;

            ENSURE(_ping_connection[from_id]);
        }

        void user_service::send_ping_address(us::user_info_ptr c, bool send_back)
        {
            INVARIANT(_user);
            INVARIANT(mail());

            std::cerr << "sending ping address " << _user->info().address() << " to " << c->address() << " port " << _ping_port << std::endl;
            //make pinhole
            if(_stun)
            {
                make_pinhole(c->address(), _stun->external_port());
                make_pinhole(c->address(), _ping_port);
            }

            ping_request a{c->address(), _user->info().id(), _user->info().address(), _ping_port, send_back};
            mail()->push_outbox(convert(a));

        }

        void user_service::fire_new_contact_event(const std::string& id)
        {
            event::new_contact e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_connected_event(const std::string& id)
        {
            event::contact_connected e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_disconnected_event(const std::string& id)
        {
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
