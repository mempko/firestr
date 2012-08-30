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
#include "util/dbc.hpp"

#include <stdexcept>
#include <thread>

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace user
    {
        namespace
        {
            const std::string SERVICE_ADDRESS = "uservice";
            const std::string ADD_REQUEST = "add_user_request";
            const std::string REQUEST_CONFIRMED = "add_user_confirmed";
            const std::string REQUEST_REJECTED = "add_user_rejected";
            const double THREAD_SLEEP = 200; //in milliseconds 
        }

        struct req_rejected 
        {
            std::string address;
        };

        struct req_confirmed
        {
            std::string to;
            user_info_ptr from;
        };

        m::message convert(const add_request& r)
        {
            REQUIRE(r.from);

            m::message m;
            m.meta.type = ADD_REQUEST;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.data = u::encode(*r.from);
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

            ENSURE(r.from);
        }

        m::message convert(const req_confirmed& r)
        {
            REQUIRE(r.from);

            m::message m;
            m.meta.type = REQUEST_CONFIRMED;
            m.meta.to = {r.to, SERVICE_ADDRESS};
            m.data = u::encode(*r.from);
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

            ENSURE(r.from);
        }

        m::message convert(const req_rejected& r)
        {
            m::message m;
            m.meta.type = REQUEST_REJECTED;
            m.meta.to = {r.address, SERVICE_ADDRESS}; 
        }

        void convert(const m::message& m, req_rejected& r)
        {
            REQUIRE_EQUAL(m.meta.type, REQUEST_REJECTED);
            REQUIRE_GREATER(m.meta.from.size(), 1);

            r.address = m.meta.from.front();
        }

        void user_service_thread(user_service* s)
        {
            REQUIRE(s);
            REQUIRE(s->_mail);
            REQUIRE(s->_user);

            while(!s->_done)
            try
            {
                m::message m;
                if(!s->_mail->pop_inbox(m))
                {
                    u::sleep_thread(THREAD_SLEEP);
                    continue;
                }

                if(m.meta.type == ADD_REQUEST)
                {
                    add_request r;
                    convert(m, r);

                    CHECK(r.from);

                    //if contact already exists send confirmation
                    //otherwise add to pending requests

                    auto f = s->_user->contact_by_id(r.from->id());
                    if(f) s->send_confirmation(f->id());
                    else 
                    {
                        u::mutex_scoped_lock l(s->_mutex);
                        s->_pending_requests[r.from->id()] = r;
                    }
                }
                else if(m.meta.type == REQUEST_CONFIRMED)
                {
                    req_confirmed r;
                    convert(m, r);

                    CHECK(r.from);

                    if(s->_sent_requests.count(r.from->address()))
                        s->confirm_user(r.from);
                }
                else if(m.meta.type == REQUEST_REJECTED)
                {
                    req_rejected r;
                    convert(m, r);
                    s->_sent_requests.erase(r.address);
                }
                else
                {
                    throw std::runtime_error{"unsuported message type `" + m.meta.type +"'"};
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "Error recieving message for mailbox " << s->_mail->address() << ". " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "Unknown error recieving message for mailbox " << s->_mail->address() << std::endl;
            }
        }

        user_service::user_service(const std::string& home) :
            _home{home}
        {
            REQUIRE_FALSE(home.empty());

            _user = load_user(home); 
            if(!_user) throw std::runtime_error{"no user found at `" + home + "'"};

            _mail.reset(new m::mailbox{SERVICE_ADDRESS});

            //start user thread
            _thread.reset(new std::thread{user_service_thread, this});

            INVARIANT(_user);
            INVARIANT(_mail);
            INVARIANT(_thread);
        }

        user_service::~user_service()
        {
            INVARIANT(_thread);

            _done = true;
            _thread->join();
        }

        message::mailbox_ptr user_service::mail()
        {
            ENSURE(_mail);
            return _mail;
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
            INVARIANT(_mail);
            REQUIRE(contact);

            //remove from sent request list
            _sent_requests.erase(contact->address());

            //add user
            _user->add_contact(contact);
            save_user(_home, *_user);
        }

        add_requests user_service::pending_requests() const
        {
            return _pending_requests;
        }

        void user_service::attempt_to_add_user(const std::string& address)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(_mail);

            std::string ex = m::external_address(address);

            user_info_ptr self{new user_info{_user->info()}};
            add_request r{ex, self};

            _sent_requests.insert(ex);
            _mail->push_outbox(convert(r));
        }

        void user_service::send_confirmation(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(_mail);

            auto p = _pending_requests.find(id);
            if(p != _pending_requests.end())
            {
                CHECK(p->second.from);

                _user->add_contact(p->second.from);
                save_user(_home, *_user);

                _pending_requests.erase(id);
            }

            user_info_ptr user = _user->contact_by_id(id);
            CHECK(user);

            user_info_ptr self{new user_info{_user->info()}};
            req_confirmed r{user->address(), self};
            _mail->push_outbox(convert(r));
        }

        void user_service::send_rejection(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT(_user);
            INVARIANT(_mail);

            //get user who wanted to be added
            auto p = _pending_requests.find(id);
            auto user = p->second.from;

            //remove request
            if(p != _pending_requests.end())
                _pending_requests.erase(id);

            req_rejected r{user->address()};
            _mail->push_outbox(convert(r));
        }

    }
}
