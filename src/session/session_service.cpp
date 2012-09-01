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

#include "session/session_service.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

#include <stdexcept>

namespace s = fire::service;
namespace u = fire::util;
namespace us = fire::user;
namespace m = fire::message;
namespace ms = fire::messages;

namespace fire
{
    namespace session
    {
        namespace 
        {
            const std::string SERVICE_ADDRESS = "session_service";
            const std::string NEW_SESSION = "new_session";
        }

        struct new_session
        {
            std::string from_id;
            std::string session_id;
        };

        m::message convert(const new_session& s)
        {
            REQUIRE_FALSE(s.session_id.empty());

            m::message m;
            m.meta.type = NEW_SESSION;
            m.data = u::to_bytes(s.session_id);

            return m;
        }

        void convert(const m::message& m, new_session& s)
        {
            REQUIRE_EQUAL(m.meta.type, NEW_SESSION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.session_id = u::to_str(m.data);
        }

        session_service::session_service(
                message::post_office_ptr post,
                user::user_service_ptr user_service) :
            s::service{SERVICE_ADDRESS},
            _post{post},
            _user_service{user_service}
        {
            REQUIRE(post);
            REQUIRE(user_service);
            REQUIRE(mail());

            _sender.reset(new ms::sender{_user_service, mail()});

            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);
        }

        void session_service::message_recieved(const message::message& m)
        {
            INVARIANT(mail());
            INVARIANT(_user_service);

            if(m.meta.type == NEW_SESSION)
            {
                new_session s;
                convert(m, s);

                us::users contacts;
                auto c = _user_service->user().contact_by_id(s.from_id);
                if(!c) return;

                contacts.push_back(c);
                create_session(s.session_id, contacts);
            }
            else
            {
                throw std::runtime_error(SERVICE_ADDRESS + " recieved unknown message type `" + m.meta.type + "'");
            }
        }

        session_ptr session_service::create_session(const std::string& id, const user::users& users)
        {
            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);

            //session already exists, abort
            if(_sessions.count(id)) return nullptr;

            //create new session
            session_ptr s{new session{id, _user_service}};
            _sessions[id] = s;

            //add new session to post office
            _post->add(s->mail());

            //send request to all users in session
            new_session ns;
            ns.session_id = id; 

            auto m = convert(ns);
            for(auto u : users)
            {
                CHECK(u);
                s->contacts().push_back(u);
                _sender->send(u->id(), m);
            }

            ENSURE(session_by_id(id));
            return s;
        }

        session_ptr session_service::create_session(const std::string& id)
        {
            us::users nobody;
            return create_session(id, nobody);
        }

        session_ptr session_service::create_session(user::users& users)
        {
            std::string id = u::uuid();
            auto sp = create_session(id, users);

            ENSURE(sp);
            return sp;
        }

        session_ptr session_service::create_session()
        {
            us::users nobody;
            auto sp =  create_session(nobody);

            ENSURE(sp);
            return sp;
        }

        session_ptr session_service::session_by_id(const std::string& id)
        {
            auto s = _sessions.find(id);
            return s != _sessions.end() ? s->second : nullptr;
        }

        bool session_service::add_contact_to_session(
                const us::user_info_ptr contact, 
                const std::string& session_id)
        {
            REQUIRE(contact);
            REQUIRE_FALSE(session_id.empty());
            INVARIANT(_sender);

            auto s = session_by_id(session_id);
            if(!s) return false;

            //add contact to session
            s->contacts().push_back(contact);

            //send request to all users in session
            new_session ns;
            ns.session_id = s->id(); 

            _sender->send(contact->id(), convert(ns));

            return true;
        }
    }
}

