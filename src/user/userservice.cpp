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

#include "user/userservice.hpp"
#include "messages/greeter.hpp"
#include "messages/pinhole.hpp"
#include "network/boost_asio.hpp"
#include "util/uuid.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <ctime>
#include <thread>
#include <random>
#include <fstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace n = fire::network;
namespace u = fire::util;
namespace us = fire::user;
namespace s = fire::service;
namespace sc = fire::security;

namespace fire
{
    namespace user
    {
        namespace
        {
            const std::string SERVICE_ADDRESS = "user_service";
            const std::string PING_REQUEST = "ping_request";
            const std::string REGISTER_WITH_GREETER = "reg_with_greeter";
            const std::string PING = "!";
            const size_t PING_THREAD_SLEEP = 500; //half a second
            const size_t PING_TICKS = 6; //3 seconds
            const size_t PING_THRESH = 3*PING_TICKS; 
            const size_t RECONNECT_TICKS = 60; //send reconnect every minute
            const size_t RECONNECT_THREAD_SLEEP = 1000; //one second
            const char CONNECTED = 'c';
            const char DISCONNECTED = 'd';
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
            u::bytes public_secret;
            int send_back;
        };

        m::message convert(const ping_request& r)
        {
            m::message m;
            m.meta.type = PING_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.meta.extra["from_id"] = r.from_id;
            m.meta.extra["send_back"] = r.send_back;
            m.meta.extra["public_secret"] = r.public_secret;

            return m;
        }

        void convert(const m::message& m, ping_request& r)
        {
            REQUIRE_EQUAL(m.meta.type, PING_REQUEST);
            REQUIRE_GREATER(m.meta.from.size(), 1);
            r.from_id = m.meta.extra["from_id"].as_string();
            r.from_ip = m.meta.extra["from_ip"].as_string();
            r.from_port = m.meta.extra["from_port"].as_string();
            r.public_secret = m.meta.extra["public_secret"].as_bytes();
            r.send_back = m.meta.extra["send_back"];
        }

        struct register_with_greeter
        {
            std::string server;
            std::string pub_key;
        };

        m::message convert(const register_with_greeter& r)
        {
            m::message m;
            m.meta.type = REGISTER_WITH_GREETER;
            m.meta.extra["server"] = r.server;
            m.meta.extra["pub_key"] = r.pub_key;
            m.meta.to = {SERVICE_ADDRESS}; 

            return m;
        }

        void convert(const m::message& m, register_with_greeter& r)
        {
            REQUIRE_EQUAL(m.meta.type, REGISTER_WITH_GREETER);
            REQUIRE_FALSE(m.meta.from.empty());

            r.server = m.meta.extra["server"].as_string();
            r.pub_key = m.meta.extra["pub_key"].as_string();
        }

        user_service::user_service(user_service_context& c) :
            s::service{SERVICE_ADDRESS, c.events},
            _user{c.user},
            _home{c.home},
            _in_host{c.host},
            _in_port{c.port},
            _session_library{c.session_library},
            _done{false}
        {
            REQUIRE_FALSE(c.home.empty());

            if(!_user) throw std::runtime_error{"no user found at `" + _home + "'"};
            update_address(n::make_udp_address(c.host, c.port));

            for(auto c : _user->contacts().list())
                add_contact_data(c);

            init_ping();
            init_reconnect();

            INVARIANT(_user);
            INVARIANT(_session_library);
            INVARIANT(mail());
            ENSURE(_ping_thread);
        }

        user_service::~user_service()
        {
            INVARIANT(_ping_thread);
            INVARIANT(_reconnect_thread);
            send_ping(DISCONNECTED);
            _done = true;
            _ping_thread->join();
            _reconnect_thread->join();
        }

        void user_service::add_contact_data(user::user_info_ptr u)
        {
            REQUIRE(u);
            contact_data cd  = {contact_data::OFFLINE, u, PING_THRESH};
            _contacts[u->id()] = cd; 

            REQUIRE(_contacts[u->id()].contact == u);
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

        void reconnect_thread(user_service* s);
        void user_service::init_reconnect()
        {
            REQUIRE(!_reconnect_thread);

            _reconnect_thread.reset(new std::thread{reconnect_thread, this});

            ENSURE(_reconnect_thread);
        }

        void user_service::reconnect()
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);

            LOG << "connecting to peers..." << std::endl;

            //send ping requests for all contacts
            //to handle local contacts on a network using 
            //last known ip/port
            send_ping_requests();

            //register with greeter
            for(const auto& g : _user->greeters())
                request_register(g);
        }

        void user_service::request_register(const greet_server& g)
        {
            auto s = n::make_tcp_address(g.host(), g.port(), _in_port);
            register_with_greeter r{s, g.public_key()};

            m::message m = convert(r);
            m.meta.to = {SERVICE_ADDRESS};
            mail()->push_outbox(m);
        }

        void user_service::find_contact_with_greeter(user_info_ptr c, const std::string& greeter)
        {
            REQUIRE(c);
            REQUIRE_FALSE(greeter.empty());
            INVARIANT(_user);

            ms::greet_find_request r {_user->info().id(), c->id()};

            //send request
            m::message m = r;
            m.meta.to = {greeter, "outside"};
            mail()->push_outbox(m);
        }

        void user_service::find_contact(user_info_ptr c)
        {
            REQUIRE(c);
            INVARIANT(_user);

            for(const auto& g : _user->greeters())
            {
                auto s = n::make_tcp_address(g.host(), g.port(), _in_port);
                CHECK_FALSE(s.empty());
                find_contact_with_greeter(c, s);
            }
        }

        void user_service::do_regiser_with_greeter(const std::string& server, const std::string& pub_key)
        {
            //regiser with greeter
            LOG << "sending greet message to " << server << std::endl;
            ms::greet_register gr
            {
                _user->info().id(), 
                {_in_host, _in_port},
                _user->info().key().key(),
                SERVICE_ADDRESS
            };

            //create security session
            _session_library->create_session(server, pub_key);

            //send registration request to greeter
            m::message gm = gr;
            gm.meta.to = {server, "outside"};
            mail()->push_outbox(gm);

            //send search query to greeter for all contacts that are offline
            for(auto c : _user->contacts().list())
            {
                CHECK(c);
                if(contact_available(c->id())) continue;
                find_contact_with_greeter(c, server);
            }
        }

        bool available(size_t ticks) { return ticks <= PING_THRESH; }

        void user_service::setup_security_session(
                const std::string& address, 
                const sc::public_key& key, 
                const u::bytes& public_val)
        {
            REQUIRE_FALSE(address.empty());
            INVARIANT(_session_library);

            _session_library->create_session(address, key, public_val);
        }

        void user_service::message_recieved(const message::message& m)
        {
            if(m.meta.type == PING)
            {
                ping r;
                convert(m, r);

                bool fire_event = false;
                bool cur_state = false;
                {
                    u::mutex_scoped_lock l(_ping_mutex);

                    auto p = _contacts.find(r.from_id);
                    if(p == _contacts.end() || !p->second.contact) return;

                    auto& ticks = p->second.last_ping;
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
                    cur_state = available(ticks);
                    fire_event = cur_state != prev_state || p->second.state == contact_data::CONNECTING;
                }

                if(fire_event)
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

                //we are already connected to this contact
                //send ping and return
                if(contact_available(c->id()) ||_contacts[c->id()].state == contact_data::CONNECTING) 
                {
                    LOG << "got connection request from: " << c->name() << " (" << c->address() << "), already connecting..." << std::endl;
                    if(r.send_back) send_ping_request(c, false);
                    return;
                }

                LOG << "got connection request from: " << c->name() << " ( old: " << c->address() << ", new: " << r.from_ip << ":" << r.from_port << "), sending ping back " << std::endl;

                //update contact address to the one specified
                //if it is different.
                update_contact_address(c->id(), r.from_ip, r.from_port);

                contact_connecting(c->id());
                if(r.send_back) send_ping_request(c, false);

                //update session to use DH 
                auto address = n::make_udp_address(r.from_ip, r.from_port);
                setup_security_session(address, c->key(), r.public_secret);

                send_ping_to(CONNECTED, c->id(), true);
            }
            else if(m.meta.type == REGISTER_WITH_GREETER)
            {
                register_with_greeter r;
                convert(m, r);
                do_regiser_with_greeter(r.server, r.pub_key);
            }
            else if(m.meta.type == ms::GREET_KEY_RESPONSE)
            {
                ms::greet_key_response rs{m};
                add_greeter(rs.host(), rs.port(), rs.key());
            }
            else if(m.meta.type == ms::GREET_FIND_RESPONSE)
            {
                ms::greet_find_response rs{m};
                if(!rs.found()) return;

                LOG << "got greet response: " << rs.external().ip << ":" << rs.external().port << std::endl;

                //send ping request using new local and remote address
                auto c = _user->contacts().by_id(rs.id());
                if(c) 
                {
                    CHECK(_contacts.count(c->id()));

                    if(_contacts[c->id()].state == contact_data::CONNECTED) return;

                    auto local = n::make_udp_address(rs.local().ip, rs.local().port);
                    auto external = n::make_udp_address(rs.external().ip, rs.external().port);
                    //create security sessions for local and external
                    _session_library->create_session(local, c->key());
                    _session_library->create_session(external, c->key());

                    //send ping request via local network and external
                    //network. First one to arrive gets to be connection
                    send_ping_request(local);
                    send_ping_request(external);
                }
            }
            else
            {
                throw std::runtime_error{"unsuported message type `" + m.meta.type +"'"};
            }
        }

        void user_service::update_contact_address(const std::string& id, const std::string& ip, const std::string& port)
        {
            INVARIANT(_user);
            INVARIANT(_session_library);
            u::mutex_scoped_lock l(_mutex);

            auto c = _user->contacts().by_id(id);
            if(!c) return;

            auto a = n::make_udp_address(ip, port);

            _session_library->create_session(a, c->key());

            if(c->address() == a) return;

            LOG << "updating address from: " << c->address() << " to: " << a << std::endl;
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

        const std::string& user_service::in_host() const
        {
            ENSURE_FALSE(_in_host.empty());
            return _in_host;
        }

        const std::string& user_service::in_port() const
        {
            ENSURE_FALSE(_in_port.empty());
            return _in_port;
        }

        void user_service::remove_contact(const std::string& id)
        {
            INVARIANT(_user);
            INVARIANT(_session_library);
            auto c = _user->contacts().by_id(id);
            if(!c) return;
            
            //remove security session
            _session_library->remove_session(c->address());

            //remove contact
            fire_contact_disconnected_event(id);
            _user->contacts().remove(c);

            save_user(_home, *_user);
        }

        void user_service::add_greeter(const std::string& address)
        {
            ms::greet_key_request r{SERVICE_ADDRESS};

            auto host_port = n::parse_host_port(address);
            auto service = n::make_tcp_address(host_port.first, host_port.second, _in_port);

            m::message m = r;
            m.meta.to = {service, "outside"};
            mail()->push_outbox(m);
        }

        void user_service::add_greeter(const std::string& host, const std::string& port, const std::string& pub_key)
        {
            INVARIANT(_user);
            if(host.empty() || port.empty() || pub_key.empty()) return;

            //check to make sure the greeter has not been added already
            for(auto g : _user->greeters())
                if(g.host() == host && g.port() == port)
                    return;

            //add greeter
            greet_server gs{host, port, pub_key};
            _user->greeters().push_back(gs);

            save_user(_home, *_user);

            //try to connect
            request_register(gs);
        }

        void user_service::remove_greeter(const std::string& address)
        {
            INVARIANT(_user);
            //parse host/port
            auto host_port = n::parse_host_port(address);

            //find greeter
            greet_servers::iterator g = _user->greeters().end();
            for(greet_servers::iterator i = _user->greeters().begin(); i != _user->greeters().end(); i++)
                if(host_port.first == i->host() && host_port.second == i->port())
                    g = i;
            if(g == _user->greeters().end()) return;
           
            _user->greeters().erase(g);

            save_user(_home, *_user);
        }

        void reconnect_thread(user_service* s)
        try
        {
            //start by connecting
            size_t ticks = RECONNECT_TICKS;
            while(!s->_done)
            try
            {
                if(ticks >= RECONNECT_TICKS) 
                {
                    s->reconnect();
                    ticks = 0;
                }
                ticks++;
                u::sleep_thread(RECONNECT_THREAD_SLEEP);
            }
            catch(std::exception& e)
            {
                LOG << "Error in reconnect thread: " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "Unexpected error in reconnect thread." << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: user_service::reconnect_thread" << std::endl;
        }

        void ping_thread(user_service* s)
        try
        {
            size_t send_ticks = 0;
            size_t reconnect_ticks = 0;
            REQUIRE(s);
            REQUIRE(s->_user);
            REQUIRE(s->_session_library);
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
                    for(auto& p : s->_contacts)
                    {
                        //skip any offline contacts
                        if(p.second.state == contact_data::OFFLINE || !p.second.contact) continue;

                        auto& ticks = p.second.last_ping;
                        bool prev_state = available(ticks);
                        ticks++;
                        bool cur_state = available(ticks);

                        //if we changed state, fire event
                        if(cur_state != prev_state)
                        {
                            CHECK(!cur_state); // should only flip one way
                            CHECK(p.second.contact);

                            s->fire_contact_disconnected_event(p.first);
                        }
                    }
                }
                u::sleep_thread(PING_THREAD_SLEEP);
            }
            catch(std::exception& e)
            {
                LOG << "Error in ping thread: " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "Unexpected error in ping thread." << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: user_service::ping_thread" << std::endl;
        }

        user_info_ptr user_service::by_id(const std::string& id) const
        {
            return _user->contacts().by_id(id);
        }

        bool user_service::contact_available(const std::string& id) const
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = _contacts.find(id);
            if(c == _contacts.end() || !c->second.contact) return false;
            return c->second.state == contact_data::CONNECTED;
        }

        void user_service::send_ping_to(char s, const std::string& id, bool force)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto& p  = _contacts[id];
            if(!p.contact || (p.state == contact_data::OFFLINE && !force)) return;
            if(!_user->contacts().by_id(p.contact->id()))
            {
                p.contact.reset();
                return;
            }
            auto c = p.contact;
            CHECK(c);

            ping r = {c->address(), _user->info().id(), s};
            mail()->push_outbox(convert(r));
        }

        void user_service::send_ping(char s)
        {
            REQUIRE(s == CONNECTED || s == DISCONNECTED);

            //send ping message to all connected contacts
            for(auto p : _contacts)
                send_ping_to(s, p.first);
        }

        void user_service::send_ping_requests()
        {
            INVARIANT(_user);

            for(auto c : _user->contacts().list())
            {
                CHECK(c);
                if(contact_available(c->id())) continue;
                send_ping_request(c);
            }
        }

        void user_service::send_ping_request(const std::string& address, bool send_back)
        {
            INVARIANT(_user);
            INVARIANT(mail());

            const auto& s = _session_library->get_session(address);
            ping_request a
            {
                address, 
                _user->info().id(), "", "", //the ip and port will be filled in on other side
                s.shared_secret.public_value(),
                send_back
            };

            //we need to force using PK encryption here because of DH timing
            //to setup the shared key
            auto m = convert(a);
            m.meta.force = m::metadata::security::assymetric;
            mail()->push_outbox(m);
        }

        void user_service::send_ping_request(us::user_info_ptr c, bool send_back)
        {
            REQUIRE(c);
            INVARIANT(_user);
            INVARIANT(mail());

            LOG << "sending connection request to " << c->name() << " (" << c->id() << ", " << c->address() << ")" << std::endl;
            _session_library->create_session(c->address(), c->key());
            send_ping_request(c->address(), send_back);
        }

        bool user_service::contact_connecting(const std::string& id)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = _user->contacts().by_id(id);
            if(!c) return false;

            auto& cd = _contacts[c->id()];
            //don't fire if state is already connected
            if(cd.state == contact_data::CONNECTING) return false;

            cd.contact = c;
            cd.last_ping = 0;
            cd.state = contact_data::CONNECTING;
            ENSURE(cd.state == contact_data::CONNECTING);
            return true;

        }

        bool user_service::contact_connected(const std::string& id)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = _user->contacts().by_id(id);
            if(!c) return false;

            auto& cd = _contacts[c->id()];
            //don't fire if state is already connected
            if(cd.state == contact_data::CONNECTED) return false;

            cd.contact = c;
            cd.last_ping = 0;
            cd.state = contact_data::CONNECTED;
            ENSURE(cd.state == contact_data::CONNECTED);
            return true;

        }

        void user_service::fire_contact_connected_event(const std::string& id)
        {
            bool state_changed = contact_connected(id);
            if(_contacts[id].state == contact_data::CONNECTING) LOG << "goop~~~~~~~~~~~~~~~~" << (state_changed ? "true" : " false") << std::endl;
            if(!state_changed) return;
            LOG << "hhooop" << std::endl;

            event::contact_connected e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_disconnected_event(const std::string& id)
        {
            auto& cd = _contacts[id];
            CHECK(cd.contact);

            auto prev_state = cd.state;

            cd.state = contact_data::OFFLINE;
            cd.last_ping = PING_THRESH;

            if(prev_state == contact_data::CONNECTED)
            {
                _session_library->remove_session(cd.contact->address());
                event::contact_disconnected e{id, cd.contact->name()};
                send_event(event::convert(e));
            }

            ENSURE(cd.state == contact_data::OFFLINE);
        }

        bool load_contact_file(const std::string& file, contact_file& cf)
        {
            std::ifstream in(file.c_str());
            if(!in.good()) 
            {
                return false;
            }

            user_info u;
            in >> u;

            u::value gv;
            in >> gv;

            cf.contact = u;
            cf.greeter = gv.as_string();
            return true;
        }

        bool save_contact_file(const std::string& file, const contact_file& cf)
        {
            std::ofstream o(file.c_str());
            if(!o.good()) return false;

            o << cf.contact;
            o << u::value{cf.greeter};

            return true;
        }

        void user_service::confirm_contact(const contact_file& cf)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);
            INVARIANT(mail());

            //check to make sure user isn't already added
            //and that user isn't self
            auto id = cf.contact.id();
            if(id == _user->info().id() || _user->contacts().by_id(id))
                return;

            user_info_ptr contact{new user_info{cf.contact}};

            //add user
            _user->contacts().add(contact);
            add_contact_data(contact);
            save_user(_home, *_user);

            //add greeter
            if(!cf.greeter.empty()) add_greeter(cf.greeter);

            //find contact address
            find_contact(contact);
            send_ping_request(contact, true);
        }

        namespace event
        {
            const std::string CONTACT_CONNECTED = "contact_con";
            const std::string CONTACT_DISCONNECTED = "contact_discon";

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
                m.meta.extra["name"] = c.name;
                m.data = u::to_bytes(c.id);
                return m;
            }

            void convert(const m::message& m, contact_disconnected& c)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_DISCONNECTED);
                c.id = u::to_str(m.data);
                c.name = m.meta.extra["name"].as_string();
            }
        }
    }
}
