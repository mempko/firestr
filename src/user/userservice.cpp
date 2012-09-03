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
#include "util/dbc.hpp"

#include <stdexcept>
#include <thread>

namespace m = fire::message;
namespace u = fire::util;
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

        user_service::user_service(const std::string& home) :
            s::service{SERVICE_ADDRESS},
            _home{home}
        {
            REQUIRE_FALSE(home.empty());

            _user = load_user(home); 
            if(!_user) throw std::runtime_error{"no user found at `" + home + "'"};

            INVARIANT(_user);
            INVARIANT(mail());
        }

        void user_service::message_recieved(const message::message& m)
        {
            if(m.meta.type == ADD_REQUEST)
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
                    confirm_user(r.from);
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

        void user_service::confirm_user(user_info_ptr contact)
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

        void user_service::attempt_to_add_user(const std::string& address)
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
    }
}
