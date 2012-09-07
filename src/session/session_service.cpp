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

        typedef std::set<std::string> contact_ids;

        u::array convert(const contact_ids& ids)
        {
            u::array a;
            for(auto id : ids) a.add(id);
            return a;
        }

        void convert(const u::array& a, contact_ids& ids)
        {
            for(auto v : a) ids.insert(v.as_string());
        }

        struct new_session
        {
            std::string from_id;
            std::string session_id;
            contact_ids ids;
        };

        m::message convert(const new_session& s)
        {
            REQUIRE_FALSE(s.session_id.empty());

            m::message m;
            m.meta.type = NEW_SESSION;
            m.meta.extra["session_id"] = s.session_id;
            m.data = u::encode(convert(s.ids));

            return m;
        }

        void convert(const m::message& m, new_session& s)
        {
            REQUIRE_EQUAL(m.meta.type, NEW_SESSION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.session_id = m.meta.extra["session_id"].as_string();
            u::array a;
            u::decode(m.data, a);
            convert(a, s.ids);
        }

        session_service::session_service(
                message::post_office_ptr post,
                user::user_service_ptr user_service,
                message::mailbox_ptr event) :
            s::service{SERVICE_ADDRESS, event},
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

                auto c = _user_service->user().contacts().by_id(s.from_id);
                if(!c) return;

                us::users contacts;
                contacts.push_back(c);

                //also get other contacts in the session and add them
                //only if the user knows them
                for(auto id : s.ids)
                {
                    auto oc = _user_service->user().contacts().by_id(id);
                    if(!oc) continue;

                    contacts.push_back(oc);
                }

                create_session(s.session_id, contacts);
            }
            else
            {
                throw std::runtime_error(SERVICE_ADDRESS + " recieved unknown message type `" + m.meta.type + "'");
            }
        }

        session_ptr session_service::create_session(const std::string& id, const user::contact_list& contacts)
        {
            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);

            //session already exists, add to existing session
            auto sp = _sessions.find(id);
            if(sp != _sessions.end())
            {
                auto session = sp->second;
                for(auto u : contacts.list())
                {
                    CHECK(u);
                    add_contact_to_session(u, session);
                }

                return sp->second;
            }

            //create new session
            session_ptr s{new session{id, _user_service, _post}};
            _sessions[id] = s;

            //add new session to post office
            _post->add(s->mail());

            //send request to all users in session
            new_session ns;
            ns.session_id = id; 

            auto m = convert(ns);
            for(auto u : contacts.list())
            {
                CHECK(u);
                s->contacts().add(u);
                _sender->send(u->id(), m);
            }

            //done creating session, fire event
            fire_new_session_event(id);

            ENSURE(session_by_id(id));
            return s;
        }

        session_ptr session_service::create_session(const std::string& id)
        {
            us::users nobody;
            return create_session(id, nobody);
        }

        session_ptr session_service::create_session(user::contact_list& contacts)
        {
            std::string id = u::uuid();
            auto sp = create_session(id, contacts);

            ENSURE(sp);
            return sp;
        }

        session_ptr session_service::create_session()
        {
            us::contact_list nobody;
            auto sp =  create_session(nobody);

            ENSURE(sp);
            return sp;
        }

        session_ptr session_service::session_by_id(const std::string& id)
        {
            auto s = _sessions.find(id);
            return s != _sessions.end() ? s->second : nullptr;
        }

        void session_service::sync_session(session_ptr s)
        {
            REQUIRE(s);

            //get current contacts in the session.
            //these will be sent to the contact 
            contact_ids ids;
            for(auto c : s->contacts().list()) 
            {
                CHECK(c);
                ids.insert(c->id());
            }

            //send request to all contacts in session
            new_session ns;
            ns.session_id = s->id(); 
            ns.ids = ids;

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), convert(ns));
            }
        }

        void session_service::sync_session(const std::string& session_id)
        {
            auto s = session_by_id(session_id);
            if(!s) return;

            sync_session(s);
        }

        void session_service::add_contact_to_session( 
                const user::user_info_ptr contact, 
                session_ptr s)
        {
            REQUIRE(contact);
            REQUIRE(s);
            INVARIANT(_sender);

            if(s->contacts().by_id(contact->id())) return;

            //add contact to session
            s->contacts().add(contact);

            sync_session(s);
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

            add_contact_to_session(contact, s);
            return true;
        }

        user::user_service_ptr session_service::user_service()
        {
            ENSURE(_user_service);
            return _user_service;
        }

        namespace event
        {
            const std::string NEW_SESSION = "new_session";

            m::message convert(const new_session& s)
            {
                m::message m;
                m.meta.type = NEW_SESSION;
                m.data = u::to_bytes(s.session_id);
                return m;
            }

            void convert(const m::message& m, new_session& s)
            {
                REQUIRE_EQUAL(m.meta.type, NEW_SESSION);
                s.session_id = u::to_str(m.data);
            }
        }

        void session_service::fire_new_session_event(const std::string id)
        {
            event::new_session e{id};
            send_event(event::convert(e));
        }
    }
}

