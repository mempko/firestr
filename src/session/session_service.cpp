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

#include "session/session_service.hpp"
#include "util/thread.hpp"
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
            const std::string SYNC_SESSION = "sync_session_msg";
            const std::string QUIT_SESSION = "quit_session_msg";
        }

        using contact_ids = std::set<std::string>;

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

        struct sync_session_msg
        {
            std::string from_id;
            std::string session_id;
            contact_ids ids;
        };

        m::message convert(const sync_session_msg& s)
        {
            REQUIRE_FALSE(s.session_id.empty());

            m::message m;
            m.meta.type = SYNC_SESSION;
            m.meta.extra["session_id"] = s.session_id;
            m.data = u::encode(convert(s.ids));

            return m;
        }

        void convert(const m::message& m, sync_session_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, SYNC_SESSION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.session_id = m.meta.extra["session_id"].as_string();
            u::array a;
            u::decode(m.data, a);
            convert(a, s.ids);
        }

        struct quit_session_msg
        {
            std::string from_id;
            std::string session_id;
        };

        m::message convert(const quit_session_msg& s)
        {
            REQUIRE_FALSE(s.session_id.empty());

            m::message m;
            m.meta.type = QUIT_SESSION;
            m.data = u::to_bytes(s.session_id);

            return m;
        }

        void convert(const m::message& m, quit_session_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, QUIT_SESSION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.session_id = u::to_str(m.data);
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

            _sender = std::make_shared<ms::sender>(_user_service, mail());

            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);
        }        

        void session_service::message_recieved(const message::message& m)
        {
            INVARIANT(mail());
            INVARIANT(_user_service);

            if(m.meta.type == SYNC_SESSION)
            {
                sync_session_msg s;
                convert(m, s);

                auto c = _user_service->user().contacts().by_id(s.from_id);
                if(!c) return;

                us::users contacts;
                contacts.push_back(c);

                auto self = _user_service->user().info().id();

                //also get other contacts in the session and add them
                //only if the user knows them
                for(auto id : s.ids)
                {
                    //skip self
                    if(id == self) continue;

                    auto oc = _user_service->user().contacts().by_id(id);
                    if(!oc) continue;

                    contacts.push_back(oc);
                }

                sync_session(s.session_id, contacts);
            }
            else if(m.meta.type == QUIT_SESSION)
            {
                quit_session_msg q;
                convert(m, q);

                auto c = _user_service->user().contacts().by_id(q.from_id);
                if(!c) return;

                auto s = session_by_id(q.session_id);
                if(!s) return;

                c = s->contacts().by_id(q.from_id);
                if(!c) return;

                s->contacts().remove(c);
                fire_contact_removed(q.session_id, q.from_id);
            }
            else
            {
                throw std::runtime_error(SERVICE_ADDRESS + " recieved unknown message type `" + m.meta.type + "'");
            }
        }

        session_ptr session_service::sync_session(const std::string& id, const user::contact_list& contacts)
        {
            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);

            u::mutex_scoped_lock l(_mutex);

            bool is_new = false;
            session_ptr s;

            auto sp = _sessions.find(id);

            //session does not exist, create it
            if(sp == _sessions.end())
            {
                //create new session
                s = std::make_shared<session>(id, _user_service, _post);
                _sessions[id] = s;

                //add new session to post office
                _post->add(s->mail());
                is_new = true;
            }
            else s = sp->second;

            CHECK(s);
            bool new_contact = false;
            for(auto c : contacts.list())
            {
                CHECK(c);
                //skip contact who is in our session
                if(s->contacts().by_id(c->id())) continue;

                s->contacts().add(c);
                new_contact = true;
            }

            //done creating session, fire event
            if(is_new) fire_new_session_event(id);

            //sync session
            if(new_contact) 
            {
                sync_existing_session(s);
                fire_session_synced_event(id);
            }

            return s;
        }

        session_ptr session_service::create_session(const std::string& id)
        {
            us::users nobody;
            auto sp = sync_session(id, nobody);
            CHECK(sp);

            sp->initiated_by_user(true);

            ENSURE(sp);
            return sp;
        }

        session_ptr session_service::create_session(user::contact_list& contacts)
        {
            std::string id = u::uuid();
            auto sp = sync_session(id, contacts);
            CHECK(sp);

            sp->initiated_by_user(true);

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

        void session_service::quit_session(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_sender);
            INVARIANT(_post);

            //find session
            auto s = _sessions.find(id);
            if(s == _sessions.end()) return;

            CHECK(s->second);

            //send quit message to contacts in the session
            quit_session_msg ns;
            ns.session_id = s->second->id(); 

            for(auto c : s->second->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), convert(ns));
            }

            //remove session from map
            _sessions.erase(s);
            _post->remove_mailbox(s->second->mail()->address());
            fire_quit_session_event(id);
        }

        session_ptr session_service::session_by_id(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);
            auto s = _sessions.find(id);
            return s != _sessions.end() ? s->second : nullptr;
        }

        void session_service::sync_existing_session(session_ptr s)
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
            sync_session_msg ns;
            ns.session_id = s->id(); 
            ns.ids = ids;

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), convert(ns));
            }
        }

        void session_service::sync_existing_session(const std::string& session_id)
        {
            auto s = session_by_id(session_id);
            if(!s) return;

            sync_existing_session(s);
        }

        void session_service::add_contact_to_session( 
                const user::user_info_ptr contact, 
                session_ptr s)
        {
            REQUIRE(contact);
            REQUIRE(s);
            INVARIANT(_sender);

            //if contact exists, we return
            if(s->contacts().by_id(contact->id())) return;

            //add contact to session
            s->contacts().add(contact);

            sync_existing_session(s);
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

        void session_service::broadcast_message(const message::message& m)
        {
            u::mutex_scoped_lock l(_mutex);
            for(auto p : _sessions)
            {
                auto session = p.second;
                CHECK(session);

                auto mail = session->mail();
                CHECK(mail);

                mail->push_inbox(m);
            }
        }

        user::user_service_ptr session_service::user_service()
        {
            ENSURE(_user_service);
            return _user_service;
        }

        namespace event
        {
            const std::string NEW_SESSION = "new_session_event";
            const std::string QUIT_SESSION = "quit_session_event";
            const std::string SESSION_SYNCED = "session_synced_event";
            const std::string CONTACT_REMOVED = "session_contact_removed";

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

            m::message convert(const quit_session& s)
            {
                m::message m;
                m.meta.type = QUIT_SESSION;
                m.data = u::to_bytes(s.session_id);
                return m;
            }

            void convert(const m::message& m, quit_session& s)
            {
                REQUIRE_EQUAL(m.meta.type, QUIT_SESSION);
                s.session_id = u::to_str(m.data);
            }

            m::message convert(const session_synced& s)
            {
                m::message m;
                m.meta.type = SESSION_SYNCED;
                m.data = u::to_bytes(s.session_id);
                return m;
            }

            void convert(const m::message& m, session_synced& s)
            {
                REQUIRE_EQUAL(m.meta.type, SESSION_SYNCED);
                s.session_id = u::to_str(m.data);
            }

            m::message convert(const contact_removed& s)
            {
                m::message m;
                m.meta.type = CONTACT_REMOVED;
                m.meta.extra["contact_id"] = s.contact_id;
                m.data = u::to_bytes(s.session_id);
                return m;
            }

            void convert(const m::message& m, contact_removed& s)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_REMOVED);
                s.session_id = u::to_str(m.data);
                s.contact_id = m.meta.extra["contact_id"].as_string();
            }
        }

        void session_service::fire_new_session_event(const std::string& id)
        {
            event::new_session e{id};
            send_event(event::convert(e));
        }

        void session_service::fire_quit_session_event(const std::string& id)
        {
            event::quit_session e{id};
            send_event(event::convert(e));
        }

        void session_service::fire_session_synced_event(const std::string& id)
        {
            event::session_synced e{id};
            send_event(event::convert(e));
        }

        void session_service::fire_contact_removed(
                const std::string& session_id, 
                const std::string& contact_id)
        {
            event::contact_removed e{session_id, contact_id};
            send_event(event::convert(e));
        }
    }
}

