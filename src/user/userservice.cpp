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

#include "user/userservice.hpp"
#include "messages/greeter.hpp"
#include "messages/pinhole.hpp"
#include "network/connection.hpp"
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
            const std::string INTRODUCTION = "contact_intro";
            const std::string PING = "!";
            const size_t PING_THREAD_SLEEP = 500; //half a second
            const size_t PING_TICKS = 6; //3 seconds
            const size_t PING_THRESH = 10*PING_TICKS; 
            const size_t RECONNECT_TICKS = 30; //send reconnect every minute
            const size_t RECONNECT_THREAD_SLEEP = 2000; //two seconds
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
            int from_port;
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
            REQUIRE_FALSE(m.meta.from.empty());
            r.from_id = m.meta.extra["from_id"].as_string();
            r.from_ip = m.meta.extra["from_ip"].as_string();
            r.from_port = m.meta.extra["from_port"].as_int();
            r.public_secret = m.meta.extra["public_secret"].as_bytes();
            r.send_back = m.meta.extra["send_back"];
        }

        struct register_with_greeter
        {
            std::string tcp_addr;
            std::string udp_addr;
            std::string pub_key;
        };

        m::message convert(const register_with_greeter& r)
        {
            m::message m;
            m.meta.type = REGISTER_WITH_GREETER;
            m.meta.extra["tcp_addr"] = r.tcp_addr;
            m.meta.extra["udp_addr"] = r.udp_addr;
            m.meta.extra["pub_key"] = r.pub_key;
            m.meta.to = {SERVICE_ADDRESS}; 

            return m;
        }

        void convert(const m::message& m, register_with_greeter& r)
        {
            REQUIRE_EQUAL(m.meta.type, REGISTER_WITH_GREETER);
            REQUIRE_FALSE(m.meta.from.empty());

            r.tcp_addr = m.meta.extra["tcp_addr"].as_string();
            r.udp_addr = m.meta.extra["udp_addr"].as_string();
            r.pub_key = m.meta.extra["pub_key"].as_string();
        }

        m::message convert(const std::string& to, const contact_introduction& in)
        {
            m::message m;
            m.meta.type = INTRODUCTION;
            m.meta.to = {to, SERVICE_ADDRESS}; 
            m.data = u::encode(from_introduction(in));
            return m;
        }

        void convert(const m::message& m, contact_introduction& r)
        {
            REQUIRE_EQUAL(m.meta.type, INTRODUCTION);
            REQUIRE_FALSE(m.meta.from.empty());
            r = to_introduction(u::decode<u::dict>(m.data));
        }

        user_service::user_service(user_service_context& c) :
            s::service{SERVICE_ADDRESS, c.events},
            _user{c.user},
            _home{c.home},
            _in_host{c.host},
            _in_port{c.port},
            _encrypted_channels{c.encrypted_channels},
            _done{false}
        {
            REQUIRE_FALSE(c.home.empty());

            init_handlers();

            if(!_user) throw std::runtime_error{"no user found at `" + _home + "'"};
            update_address(n::make_udp_address(c.host, c.port));

            for(auto c : _user->contacts().list())
                add_contact_data(c);

            init_ping();
            init_reconnect();

            start();

            INVARIANT(_user);
            INVARIANT(_encrypted_channels);
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

        void user_service::init_handlers()
        {
            using std::bind;
            using namespace std::placeholders;
            handle(PING, bind(&user_service::received_ping, this, _1));
            handle(PING_REQUEST, bind(&user_service::received_connect_request, this, _1));
            handle(REGISTER_WITH_GREETER, bind(&user_service::received_register_with_greeter, this, _1));
            handle(ms::GREET_KEY_RESPONSE, bind(&user_service::received_greet_key_response, this, _1));
            handle(ms::GREET_FIND_RESPONSE, bind(&user_service::received_greet_find_response, this, _1));
            handle(INTRODUCTION, bind(&user_service::received_introduction, this, _1));
        }

        void user_service::add_contact_data(user::user_info_ptr u)
        {
            u::mutex_scoped_lock l(_ping_mutex);
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

            //register with greeters
            for(const auto& g : _user->greeters())
                request_register(g);

            //send ping requests for all contacts
            //to handle local contacts on a network using 
            //last known ip/port
            send_ping_requests();
        }

        void user_service::request_register(const greet_server& g)
        {
            //tcp and udp register...
            auto tcp_s = n::make_tcp_address(g.host(), g.port());
            auto udp_s = n::make_udp_address(g.host(), g.port());
            register_with_greeter r{tcp_s, udp_s, g.public_key()};

            m::message m = convert(r);
            m.meta.to = {SERVICE_ADDRESS};
            mail()->push_outbox(m);
        }

        void user_service::find_contact_with_greeter(user_info_ptr c, const std::string& greeter)
        {
            REQUIRE(c);
            REQUIRE_FALSE(greeter.empty());
            INVARIANT(_user);
            REQUIRE_FALSE(is_contact_connecting(c->id()));
            REQUIRE_FALSE(contact_available(c->id()));

            ms::greet_find_request r {_user->info().id(), c->id()};

            //send request
            m::message m = r;
            m.meta.to = {greeter, "outside"};
            mail()->push_outbox(m);
        }

        void user_service::do_regiser_with_greeter(
                const std::string& tcp_addr, 
                const std::string& udp_addr, 
                const std::string& pub_key)
        {
            //regiser with greeter
            LOG << "sending greet message to " << tcp_addr << " : " << udp_addr << std::endl;
            ms::greet_register gr
            {
                _user->info().id(), 
                ms::greet_endpoint{_in_host, _in_port},
                _user->info().key().key(),
                SERVICE_ADDRESS
            };

            //create security conversation
            _encrypted_channels->create_channel(tcp_addr, pub_key);
            _encrypted_channels->create_channel(udp_addr, pub_key);

            //send registration request to greeter
            m::message tcp_gm = gr; tcp_gm.meta.to = {tcp_addr, "outside"};
            m::message udp_gm = gr; udp_gm.meta.to = {udp_addr, "outside"};
            mail()->push_outbox(tcp_gm);
            mail()->push_outbox(udp_gm);

            //send search query to greeter for all contacts that are offline
            for(auto c : _user->contacts().list())
            {
                CHECK(c);
                if(is_contact_connecting(c->id()) || contact_available(c->id())) continue;
                find_contact_with_greeter(c, tcp_addr);
            }
        }

        bool available(size_t ticks) { return ticks <= PING_THRESH; }

        void user_service::setup_security_conversation(
                const std::string& address, 
                const sc::public_key& key, 
                const u::bytes& public_val)
        {
            REQUIRE_FALSE(address.empty());
            INVARIANT(_encrypted_channels);

            _encrypted_channels->create_channel(address, key, public_val);
        }

        void user_service::received_ping(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, PING);

            m::expect_remote(m);
            m::expect_symmetric(m);

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
                else 
                {
                    u::mutex_scoped_lock l(_ping_mutex);
                    fire_contact_disconnected_event(r.from_id);
                }
            }
        }

        void user_service::received_connect_request(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, PING_REQUEST);
            m::expect_remote(m);
            m::expect_asymmetric(m);

            ping_request r;
            convert(m, r);

            auto c = by_id(r.from_id);
            if(!c) return;

            //we are already connected to this contact
            //send ping and return
            if(contact_available(c->id())) 
            {
                LOG << "got connection request from: " << c->name() << " (" << c->address() << "), already connected..." << std::endl;
                return;
            }

            if(is_contact_connecting(c->id())) 
            {
                LOG << "got connection request from: " << c->name() << " (" << c->address() << "), already connecting..." << std::endl;
                return;
            }

            LOG << "got connection request from: " << c->name() << " ( old: " << c->address() << ", new: " << r.from_ip << ":" << r.from_port << "), sending ping back " << std::endl;

            //update contact address to the one specified
            //if it is different.
            update_contact_address(c->id(), r.from_ip, r.from_port);

            if(r.send_back) send_ping_request(c, false);
            contact_connecting(c->id());

            //update conversation to use DH 
            auto address = n::make_udp_address(r.from_ip, r.from_port);
            setup_security_conversation(address, c->key(), r.public_secret);
            send_ping_to(CONNECTED, c->id(), true);
        }

        void user_service::received_register_with_greeter(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, REGISTER_WITH_GREETER);

            m::expect_local(m);

            register_with_greeter r;
            convert(m, r);
            do_regiser_with_greeter(r.tcp_addr, r.udp_addr, r.pub_key);
        }

        void user_service::received_greet_key_response(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, ms::GREET_KEY_RESPONSE);

            m::expect_remote(m);
            if(!m::is_plaintext(m) && !m::is_asymmetric(m)) return;

            ms::greet_key_response rs{m};
            add_greeter(rs.host(), rs.port(), rs.key());
        }

        void user_service::received_greet_find_response(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, ms::GREET_FIND_RESPONSE);
            m::expect_remote(m);
            m::expect_asymmetric(m);

            ms::greet_find_response rs{m};
            if(!rs.found()) return;

            //send ping request using new local and remote address
            auto c = by_id(rs.id());
            if(c) 
            {
                CHECK(_contacts.count(c->id()));
                if(contact_available(c->id()) || is_contact_connecting(c->id())) 
                {
                    LOG << "got greet response for: " << c->name() << " (" << c->id() << ") address:" << rs.external().ip << ":" << rs.external().port << " but already connecting..." << std::endl;
                    return;
                }

                auto local = n::make_udp_address(rs.local().ip, rs.local().port);
                auto external = n::make_udp_address(rs.external().ip, rs.external().port);

                LOG << "got greet response for: " << c->name() << " (" << c->id() << " ) local: " << local << " external: " << external <<  " sending requests..." << std::endl;
                //create security conversations for local 
                _encrypted_channels->create_channel(local, c->key());

                //send ping request via local network 
                send_ping_request(local);

                //if external is different from local, create a security
                //conversation and send a ping request. First one to make it
                //wins the race.
                if(external != local)
                {
                    _encrypted_channels->create_channel(external, c->key());
                    send_ping_request(external);
                }
            }
        }

        void user_service::received_introduction(const message::message& m)
        {
            m::expect_remote(m);
            m::expect_symmetric(m);

            contact_introduction i;
            convert(m, i);
            auto c = by_id(i.from_id);
            if(c) 
            {
                LOG << "got introduction from " << c->name() << " about " << i.contact.name() << "." << c->name() << " says: " << i.message << std::endl;
                auto index = add_introduction(i);
                if(index >= 0) fire_new_introduction(index);
            }
        }

        void user_service::update_contact_address(const std::string& id, const std::string& ip, n::port_type port)
        {
            INVARIANT(_user);
            INVARIANT(_encrypted_channels);
            u::mutex_scoped_lock l(_mutex);

            auto c = by_id(id);
            if(!c) return;

            auto a = n::make_udp_address(ip, port);

            _encrypted_channels->create_channel(a, c->key());

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

        n::port_type user_service::in_port() const
        {
            ENSURE_GREATER(_in_port, 0);
            return _in_port;
        }

        void user_service::remove_contact(const std::string& id)
        {
            INVARIANT(_user);
            INVARIANT(_encrypted_channels);
            auto c = by_id(id);
            if(!c) return;
            
            //remove security conversation
            _encrypted_channels->remove_channel(c->address());

            //remove contact
            fire_contact_disconnected_event(id);
            _user->contacts().remove(c);

            save_user(_home, *_user);
        }

        void user_service::add_greeter(const std::string& address)
        {
            ms::greet_key_request r{SERVICE_ADDRESS};

            auto host_port = n::parse_host_port(address);
            auto service = n::make_tcp_address(host_port.first, host_port.second);

            m::message m = r;
            m.meta.to = {service, "outside"};
            mail()->push_outbox(m);
        }

        void user_service::add_greeter(const std::string& host, n::port_type port, const std::string& pub_key)
        {
            INVARIANT(_user);
            if(host.empty() || port == 0 || pub_key.empty()) return;

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
            REQUIRE(s);
            REQUIRE(s->_user);
            REQUIRE(s->_encrypted_channels);

            size_t send_ticks = 0;

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

        bool user_service::is_contact_connecting(const std::string& id) const
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = _contacts.find(id);
            if(c == _contacts.end() || !c->second.contact) return false;
            return c->second.state == contact_data::CONNECTING;
        }

        void user_service::send_ping_to(char s, const std::string& id, bool force)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            if(_contacts.count(id) == 0) return;

            auto& p  = _contacts[id];
            if(!p.contact || (p.state == contact_data::OFFLINE && !force)) return;
            if(!by_id(p.contact->id()))
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
                if(is_contact_connecting(c->id()) || contact_available(c->id())) continue;
                send_ping_request(c);
            }
        }

        void user_service::send_ping_request(const std::string& address, bool send_back)
        {
            INVARIANT(_user);
            INVARIANT(mail());

            const auto& s = _encrypted_channels->get_channel(address);
            ping_request a
            {
                address, 
                _user->info().id(), "", 0, //the ip and port will be filled in on other side
                s.shared_secret.public_value(),
                send_back
            };

            //we need to force using PK encryption here because of DH timing
            //to setup the shared key
            auto m = convert(a);
            m.meta.encryption = m::metadata::encryption_type::asymmetric;
            mail()->push_outbox(m);
        }

        void user_service::send_ping_request(us::user_info_ptr c, bool send_back)
        {
            REQUIRE(c);
            INVARIANT(_user);
            INVARIANT(mail());
            REQUIRE_FALSE(is_contact_connecting(c->id()));
            REQUIRE_FALSE(contact_available(c->id()));

            LOG << "sending connection request to " << c->name() << " (" << c->id() << ", " << c->address() << ")" << std::endl;
            _encrypted_channels->create_channel(c->address(), c->key());
            send_ping_request(c->address(), send_back);
        }

        bool user_service::contact_connecting(const std::string& id)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = by_id(id);
            if(!c) return false;

            auto& cd = _contacts[c->id()];

            cd.contact = c;
            cd.last_ping = 0;
            cd.state = contact_data::CONNECTING;
            ENSURE(cd.state == contact_data::CONNECTING);
            return true;
        }

        bool user_service::contact_connected(const std::string& id)
        {
            u::mutex_scoped_lock l(_ping_mutex);
            auto c = by_id(id);
            if(!c) return false;

            auto& cd = _contacts[c->id()];

            //don't fire if state is already connected
            if(cd.state == contact_data::CONNECTED) return false;
            CHECK(cd.state == contact_data::CONNECTING);

            cd.contact = c;
            cd.last_ping = 0;
            cd.state = contact_data::CONNECTED;
            LOG << c->name() << " connected" << std::endl;
            ENSURE(cd.state == contact_data::CONNECTED);
            return true;

        }

        void user_service::fire_contact_connected_event(const std::string& id)
        {
            bool state_changed = contact_connected(id);
            if(!state_changed) return;

            event::contact_connected e{id};
            send_event(event::convert(e));
        }

        void user_service::fire_contact_disconnected_event(const std::string& id)
        {
            auto c = by_id(id);
            if(!c) return;

            auto& cd = _contacts[id];
            if(cd.state == contact_data::OFFLINE) return;

            CHECK(cd.contact);

            auto prev_state = cd.state;

            cd.state = contact_data::OFFLINE;
            cd.last_ping = PING_THRESH;

            if(prev_state == contact_data::CONNECTED)
            {
                LOG << cd.contact->name() << " disconnected" << std::endl;
                _encrypted_channels->remove_channel(cd.contact->address());
                event::contact_disconnected e{id, cd.contact->name()};
                send_event(event::convert(e));
            }
            else CHECK(prev_state == contact_data::CONNECTING);

            ENSURE(cd.state == contact_data::OFFLINE);
        }

        void user_service::fire_new_introduction(size_t i)
        {
            event::new_introduction n{i};
            send_event(event::convert(n));
        }

#ifdef _WIN64
        bool load_contact_file(const unsigned short* file, contact_file& cf)
        try
        {
            std::ifstream in(file, std::fstream::in | std::fstream::binary);
#else
        bool load_contact_file(const std::string& file, contact_file& cf)
        try
        {
            std::ifstream in(file.c_str(), std::fstream::in | std::fstream::binary);
#endif
            if(!in.good()) 
                return false;

            user_info u;
            in >> u;

            u::value gv;
            in >> gv;

            cf.contact = u;
            cf.greeter = gv.as_string();
            return true;
        } 
        catch(std::exception& e)
        {
            LOG << "error loading contact file `" << file << "'. " << e.what() << std::endl;
            return false;
        }

        bool save_contact_file(const std::string& file, const contact_file& cf)
        try
        {
            std::ofstream o(file.c_str(), std::fstream::out | std::fstream::binary);
            if(!o.good()) return false;

            o << cf.contact;
            o << u::value{cf.greeter};

            return true;
        }
        catch(std::exception& e)
        {
            LOG << "error saving contact file `" << file << "'. " << e.what() << std::endl;
            return false;
        }

        bool user_service::confirm_contact(const contact_file& cf)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);
            INVARIANT(mail());

            //check to make sure user isn't already added
            //and that user isn't self
            auto id = cf.contact.id();
            if(id == _user->info().id() || by_id(id))
                return false;

            user_info_ptr contact{new user_info{cf.contact}};

            //add user
            _user->contacts().add(contact);
            add_contact_data(contact);
            save_user(_home, *_user);

            //add greeter
            if(!cf.greeter.empty()) add_greeter(cf.greeter);

            CHECK_FALSE(is_contact_connecting(contact->id()));
            CHECK_FALSE(contact_available(contact->id()));
            send_ping_request(contact, true);

            return true;
        }

        int user_service::add_introduction(const contact_introduction& in)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);
            auto& is = _user->introductions();

            //make sure we already don't have the user
            auto cs = by_id(in.contact.id());
            if(cs) return -1;

            //check to make we already don't have a similar introduction
            for(const auto& i : is)
                if(i.contact.id() == in.contact.id())
                    return -1;

            //add and save
            is.push_back(in);
            save_user(_home, *_user);

            ENSURE_FALSE(is.empty());
            return is.size() - 1;
        }

        void user_service::remove_introduction(const contact_introduction& i)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);
            auto& is = _user->introductions();

            auto e = std::find(is.begin(), is.end(), i);
            if(e == is.end()) return;

            //remove and save
            is.erase(e);
            save_user(_home, *_user);
        }

        contact_introductions user_service::introductions() const
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_user);
            return _user->introductions();
        }

        void user_service::send_introduction(const std::string& to, contact_introduction& i)
        {
            REQUIRE_FALSE(to.empty());
            INVARIANT(_user);
            auto c = by_id(to);
            if(!c) return;

            LOG << "sending introduction to " << c->name() << " about " << i.contact.name() << std::endl;
            m::message m = convert(c->address(), i);
            mail()->push_outbox(m);
        }

        namespace event
        {
            const std::string CONTACT_CONNECTED = "contact_con";
            const std::string CONTACT_DISCONNECTED = "contact_discon";
            const std::string NEW_INTRODUCTION = "new_introduction";

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

            m::message convert(const new_introduction& c)
            {
                m::message m;
                m.meta.type = NEW_INTRODUCTION;
                m.meta.extra["index"] = c.index;
                return m;
            }

            void convert(const m::message& m, new_introduction& ni)
            {
                REQUIRE_EQUAL(m.meta.type, NEW_INTRODUCTION);
                ni.index = m.meta.extra["index"].as_size();
            }
        }
    }
}
