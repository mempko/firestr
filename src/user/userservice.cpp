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
#include "util/uuid.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"

#include <stdexcept>
#include <thread>

namespace m = fire::message;
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
        }

        struct ping_request 
        {
            std::string to;
            std::string from_id;
            std::string port;
            int send_back;
        };

        m::message convert(const ping_request& r)
        {
            m::message m;
            m.meta.type = PING_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["from_id"] = r.from_id;
            m.meta.extra["send_back"] = r.send_back;
            m.data = u::to_bytes(r.port);

            return m;
        }

        void convert(const m::message& m, ping_request& r)
        {
            REQUIRE_EQUAL(m.meta.type, PING_REQUEST);
            REQUIRE_GREATER(m.meta.from.size(), 1);
            r.from_id = m.meta.extra["from_id"].as_string();
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

        user_service::user_service(
                const std::string& home,
                const std::string& ping_port,
                message::mailbox_ptr event) :
            s::service{SERVICE_ADDRESS, event},
            _home{home},
            _ping_port{ping_port},
            _done{false}
        {
            REQUIRE_FALSE(home.empty());

            _user = load_user(home); 
            if(!_user) throw std::runtime_error{"no user found at `" + home + "'"};

            init_ping();
            send_ping_port_requests();

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
        }

        void ping_thread(user_service* s);
        void user_service::init_ping()
        {
            REQUIRE(!_ping_thread);
            REQUIRE(!_ping_queue);

            std::string ping_address = "zmq,tcp://*:" + _ping_port;
            n::queue_options qo = { 
                {"pub", "1"}, 
                {"bnd", "1"},
                {"block", "0"}};
            _ping_queue = n::create_message_queue(ping_address, qo);
            _ping_thread.reset(new std::thread{ping_thread, this});

            ENSURE(_ping_queue);
            ENSURE(_ping_thread);
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

                {
                    u::mutex_scoped_lock l(_ping_mutex);
                    if(_ping_connection.count(c->id())) 
                    {
                        if(r.send_back) send_ping_address(c, false);
                        return;
                    }
                }

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
                        if(queue->recieve(b)) 
                        {
                            if(b.size() == 1)
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

            ping_request a{c->address(), _user->info().id(), _ping_port, send_back};
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
