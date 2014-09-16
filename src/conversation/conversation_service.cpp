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

#include "conversation/conversation_service.hpp"
#include "messages/new_app.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/thread.hpp"
#include "util/uuid.hpp"

#include <stdexcept>

namespace s = fire::service;
namespace u = fire::util;
namespace us = fire::user;
namespace m = fire::message;
namespace ms = fire::messages;

namespace fire
{
    namespace conversation
    {
        namespace 
        {
            const std::string SERVICE_ADDRESS = "conversation_service";
            const std::string SYNC_CONVERSATION = "sync_conversation_msg";
            const std::string QUIT_CONVERSATION = "quit_conversation_msg";
            const std::string ASK_CONTACT_REQ = "ask_contact_req_msg";
            const std::string ASK_CONTACT_RES = "ask_contact_res_msg";
        }

        using id_set = std::set<std::string>;

        u::array convert(const id_set& ids)
        {
            u::array a;
            for(auto id : ids) a.add(id);
            return a;
        }

        void convert(const u::array& a, id_set& ids)
        {
            for(auto v : a) ids.insert(v.as_string());
        }

        struct sync_conversation_msg
        {
            std::string from_id;
            std::string conversation_id;
            id_set contacts;
            id_set apps;
        };

        m::message convert(const sync_conversation_msg& s)
        {
            REQUIRE_FALSE(s.conversation_id.empty());

            m::message m;
            m.meta.type = SYNC_CONVERSATION;
            m.meta.extra["conversation_id"] = s.conversation_id;
            
            u::dict d;
            d["contacts"] = convert(s.contacts);
            d["apps"] = convert(s.apps); 
            m.data = u::encode(d);

            return m;
        }

        void convert(const m::message& m, sync_conversation_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, SYNC_CONVERSATION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.conversation_id = m.meta.extra["conversation_id"].as_string();

            u::dict d;
            u::decode(m.data, d);

            convert(d["contacts"].as_array(), s.contacts);
            convert(d["apps"].as_array(), s.apps);
        }

        struct quit_conversation_msg
        {
            std::string from_id;
            std::string conversation_id;
        };

        m::message convert(const quit_conversation_msg& s)
        {
            REQUIRE_FALSE(s.conversation_id.empty());

            m::message m;
            m.meta.type = QUIT_CONVERSATION;
            m.data = u::to_bytes(s.conversation_id);

            return m;
        }

        void convert(const m::message& m, quit_conversation_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, QUIT_CONVERSATION);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.conversation_id = u::to_str(m.data);
        }

        struct ask_contact_req_msg
        {
            std::string from_id;
            std::string conversation_id;
            std::string id;
        };

        m::message convert(const ask_contact_req_msg& s)
        {
            REQUIRE_FALSE(s.conversation_id.empty());
            REQUIRE_FALSE(s.id.empty());

            m::message m;
            m.meta.type = ASK_CONTACT_REQ;
            m.meta.extra["convo_id"] = s.conversation_id;
            m.data = u::to_bytes(s.id);

            return m;
        }

        void convert(const m::message& m, ask_contact_req_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, ASK_CONTACT_REQ);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.conversation_id = m.meta.extra["convo_id"].as_string();
            s.id = u::to_str(m.data);
        }

        struct ask_contact_res_msg
        {
            std::string from_id;
            std::string id;
            std::string conversation_id;
            enum stat { KNOW, DONT_KNOW} status;
        };

        m::message convert(const ask_contact_res_msg& s)
        {
            REQUIRE_FALSE(s.conversation_id.empty());
            REQUIRE_FALSE(s.id.empty());

            m::message m;
            m.meta.type = ASK_CONTACT_RES;
            m.meta.extra["convo_id"] = s.conversation_id;
            m.data = u::to_bytes(s.id);
            int r = s.status == ask_contact_res_msg::KNOW ? 1 : 0;
            m.meta.extra["stat"] = r; 

            return m;
        }

        void convert(const m::message& m, ask_contact_res_msg& s)
        {
            REQUIRE_EQUAL(m.meta.type, ASK_CONTACT_RES);

            s.from_id = m.meta.extra["from_id"].as_string();
            s.conversation_id = m.meta.extra["convo_id"].as_string();
            s.id = u::to_str(m.data);
            int r = m.meta.extra["stat"].as_int();
            s.status = r == 1 ? ask_contact_res_msg::KNOW : ask_contact_res_msg::DONT_KNOW;
        }

        conversation_service::conversation_service(
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

        conversation_service::~conversation_service()
        {
            //copy conversations
            conversation_map conversations;
            {
                u::mutex_scoped_lock l(_mutex);
                conversations = _conversations;
            }

            for(const auto& s : conversations)
            {
                CHECK(s.second);
                quit_conversation(s.second->id());
            }
        }

        void conversation_service::message_received(const message::message& m)
        {
            INVARIANT(mail());
            INVARIANT(_user_service);
            m::expect_symmetric(m);

            if(m.meta.type == SYNC_CONVERSATION)
            {
                sync_conversation_msg s;
                convert(m, s);

                auto c = _user_service->user().contacts().by_id(s.from_id);
                if(!c) return;

                us::users contacts;
                contacts.push_back(c);

                auto self = _user_service->user().info().id();

                //also get other contacts in the conversation and add them
                //only if the user knows them
                for(auto id : s.contacts)
                {
                    //skip self
                    if(id == self) continue;

                    auto oc = _user_service->user().contacts().by_id(id);
                    if(!oc) continue;

                    contacts.push_back(oc);
                }

                sync_conversation(s.from_id, s.conversation_id, contacts, s.apps);
            }
            else if(m.meta.type == QUIT_CONVERSATION)
            {
                quit_conversation_msg q;
                convert(m, q);

                auto c = _user_service->user().contacts().by_id(q.from_id);
                if(!c) return;

                auto s = conversation_by_id(q.conversation_id);
                if(!s) return;

                c = s->contacts().by_id(q.from_id);
                if(!c) return;

                s->contacts().remove(c);
                fire_contact_removed(q.conversation_id, q.from_id);
            }
            else if(m.meta.type == ASK_CONTACT_REQ)
            {
                ask_contact_req_msg a;
                convert(m, a);

                bool hc = _user_service->user().contacts().has(a.from_id);
                if(!hc) return;

                auto s = conversation_by_id(a.conversation_id);
                if(!s) return;

                bool know = _user_service->user().contacts().has(a.id);
                ask_contact_res_msg r;
                r.conversation_id = a.conversation_id;
                r.id = a.id;
                r.status = know ? ask_contact_res_msg::KNOW : ask_contact_res_msg::DONT_KNOW;
                _sender->send(a.from_id, convert(r));
            }
            else if(m.meta.type == ASK_CONTACT_RES)
            {
                ask_contact_res_msg r;
                convert(m, r);

                auto fc = _user_service->user().contacts().by_id(r.from_id);
                if(!fc) return;

                auto c = _user_service->user().contacts().by_id(r.id);
                if(!c) return;

                auto s = conversation_by_id(r.conversation_id);
                if(!s) return;

                s->know_contact(r.id, r.from_id, 
                        r.status == ask_contact_res_msg::KNOW ? 
                        know_request::KNOW : know_request::DONT_KNOW);

                if(s->part_of_clique(r.id))
                    add_contact_to_conversation_p(c, s);
            }
            else
            {
                throw std::runtime_error(SERVICE_ADDRESS + " received unknown message type `" + m.meta.type + "'");
            }
        }

        conversation_ptr conversation_service::sync_conversation(
                const std::string& from_id,
                const std::string& id, 
                const user::contact_list& contacts,
                const app_addresses& apps)
        {
            INVARIANT(_post);
            INVARIANT(_user_service);
            INVARIANT(_sender);

            u::mutex_scoped_lock l(_mutex);

            bool is_new = false;
            conversation_ptr s;

            auto sp = _conversations.find(id);

            //conversation does not exist, create it
            if(sp == _conversations.end())
            {
                //create new conversation
                s = std::make_shared<conversation>(id, _user_service, _post);
                _conversations[id] = s;

                //add new conversation to post office
                _post->add(s->mail());
                is_new = true;
            }
            else s = sp->second;

            //add contacts to conversation
            CHECK(s);
            for(auto c : contacts.list())
            {
                CHECK(c);
                add_contact_to_conversation(c, s);
            }

            //add apps in conversation
            id_set need_apps;
            for(const auto& app : apps)
            {
                if(s->has_app(app)) continue;
                LOG << "conversation: " << s->id() << " needs app with address: " << app << std::endl;
                need_apps.insert(app);
            }

            //done creating conversation, fire event
            if(is_new) fire_new_conversation_event(id);

            //request apps that are needed
            request_apps(from_id, s, need_apps);

            return s;
        }

        void conversation_service::request_apps(
                const std::string& from_id, 
                conversation_ptr c, 
                const app_addresses& apps)
        {
            REQUIRE(c);
            for(const auto& app_address : apps)
            {
                ms::request_app n{app_address, c->id()}; 
                if(from_id.empty()) c->send(n);
                else c->send(from_id, n);
            }
        }

        conversation_ptr conversation_service::create_conversation(const std::string& id)
        {
            us::users nobody;
            std::string from_nobody = "";
            app_addresses no_apps;
            auto sp = sync_conversation(from_nobody, id, nobody, no_apps);
            CHECK(sp);

            sp->initiated_by_user(true);

            ENSURE(sp);
            return sp;
        }

        conversation_ptr conversation_service::create_conversation(user::contact_list& contacts)
        {
            std::string id = u::uuid();
            std::string from_nobody = "";
            app_addresses no_apps;
            auto sp = sync_conversation(from_nobody, id, contacts, no_apps);
            CHECK(sp);

            sp->initiated_by_user(true);

            ENSURE(sp);
            return sp;
        }

        conversation_ptr conversation_service::create_conversation()
        {
            us::contact_list nobody;
            auto sp =  create_conversation(nobody);

            ENSURE(sp);
            return sp;
        }

        void conversation_service::quit_conversation(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_sender);
            INVARIANT(_post);

            //find conversation
            auto s = _conversations.find(id);
            if(s == _conversations.end()) return;

            CHECK(s->second);

            //send quit message to contacts in the conversation
            quit_conversation_msg ns;
            ns.conversation_id = s->second->id(); 

            for(auto c : s->second->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), convert(ns));
            }

            //remove conversation from map
            _conversations.erase(s);
            fire_quit_conversation_event(id);
        }

        conversation_ptr conversation_service::conversation_by_id(const std::string& id)
        {
            u::mutex_scoped_lock l(_mutex);
            auto s = _conversations.find(id);
            return s != _conversations.end() ? s->second : nullptr;
        }

        void conversation_service::sync_existing_conversation(conversation_ptr s)
        {
            REQUIRE(s);

            //get current contacts in the conversation.
            //these will be sent to the contact 
            id_set contacts;
            for(auto c : s->contacts().list()) 
            {
                CHECK(c);
                contacts.insert(c->id());
            }

            //get apps in conversation
            id_set apps;
            for(const auto& app : s->apps())
                apps.insert(app.address);

            //send request to all contacts in conversation
            sync_conversation_msg ns;
            ns.conversation_id = s->id(); 
            ns.contacts = contacts;
            ns.apps = apps;

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), convert(ns));
            }
        }

        void conversation_service::sync_existing_conversation(const std::string& conversation_id)
        {
            auto s = conversation_by_id(conversation_id);
            if(!s) return;

            sync_existing_conversation(s);
        }

        void conversation_service::add_contact_to_conversation( 
                const user::user_info_ptr c, 
                conversation_ptr s)
        {
            REQUIRE(c);
            REQUIRE(s);

            if(s->contacts().by_id(c->id())) return;

            if(s->contacts().empty() && s->pending().empty())
            {
                add_contact_to_conversation_p(c, s);
                return;
            }

            ask_about(c->id(), s);
        }

        void conversation_service::ask_about(
                        const std::string& id,
                        conversation_ptr s)
        {
            REQUIRE_FALSE(id.empty());
            REQUIRE(s);
            INVARIANT(_sender);

            s->asked_about(id);

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                ask_contact_req_msg m;
                m.id = id;
                m.conversation_id = s->id();
                _sender->send(c->id(), convert(m));
            }
        }

        void conversation_service::add_contact_to_conversation_p( 
                const user::user_info_ptr c, 
                conversation_ptr s)
        {
            REQUIRE(c);
            REQUIRE(s);
            INVARIANT(_sender);

            //if contact exists, we return
            if(s->contacts().by_id(c->id())) return;

            //add contact to conversation
            s->contacts().add(c);

            sync_existing_conversation(s);
            fire_conversation_synced_event(s->id());
            fire_contact_added(s->id(), c->id());
        }

        void conversation_service::broadcast_message(const message::message& m)
        {
            u::mutex_scoped_lock l(_mutex);
            for(auto p : _conversations)
            {
                auto conversation = p.second;
                CHECK(conversation);

                auto mail = conversation->mail();
                CHECK(mail);

                mail->push_inbox(m);
            }
        }

        user::user_service_ptr conversation_service::user_service()
        {
            ENSURE(_user_service);
            return _user_service;
        }

        namespace event
        {
            const std::string NEW_CONVERSATION = "new_conversation_event";
            const std::string QUIT_CONVERSATION = "quit_conversation_event";
            const std::string CONVERSATION_SYNCED = "conversation_synced_event";
            const std::string CONTACT_REMOVED = "conversation_contact_removed";
            const std::string CONTACT_ADDED = "conversation_contact_added";
            const std::string CONVERSATION_ALERT = "conversation_alert";

            m::message convert(const new_conversation& s)
            {
                m::message m;
                m.meta.type = NEW_CONVERSATION;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, new_conversation& s)
            {
                REQUIRE_EQUAL(m.meta.type, NEW_CONVERSATION);
                s.conversation_id = u::to_str(m.data);
            }

            m::message convert(const quit_conversation& s)
            {
                m::message m;
                m.meta.type = QUIT_CONVERSATION;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, quit_conversation& s)
            {
                REQUIRE_EQUAL(m.meta.type, QUIT_CONVERSATION);
                s.conversation_id = u::to_str(m.data);
            }

            m::message convert(const conversation_synced& s)
            {
                m::message m;
                m.meta.type = CONVERSATION_SYNCED;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, conversation_synced& s)
            {
                REQUIRE_EQUAL(m.meta.type, CONVERSATION_SYNCED);
                s.conversation_id = u::to_str(m.data);
            }

            m::message convert(const contact_removed& s)
            {
                m::message m;
                m.meta.type = CONTACT_REMOVED;
                m.meta.extra["contact_id"] = s.contact_id;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, contact_removed& s)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_REMOVED);
                s.conversation_id = u::to_str(m.data);
                s.contact_id = m.meta.extra["contact_id"].as_string();
            }

            m::message convert(const contact_added& s)
            {
                m::message m;
                m.meta.type = CONTACT_ADDED;
                m.meta.extra["contact_id"] = s.contact_id;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, contact_added& s)
            {
                REQUIRE_EQUAL(m.meta.type, CONTACT_ADDED);
                s.conversation_id = u::to_str(m.data);
                s.contact_id = m.meta.extra["contact_id"].as_string();
            }

            message::message convert(const conversation_alert& s)
            {
                m::message m;
                m.meta.type = CONVERSATION_ALERT;
                m.meta.extra["v"] = s.visible ? 1 : 0;
                m.data = u::to_bytes(s.conversation_id);
                return m;
            }

            void convert(const m::message& m, conversation_alert& s)
            {
                REQUIRE_EQUAL(m.meta.type, CONVERSATION_ALERT);
                s.visible = m.meta.extra["v"].as_int() == 1;
                s.conversation_id = u::to_str(m.data);
            }
        }

        void conversation_service::fire_new_conversation_event(const std::string& id)
        {
            event::new_conversation e{id};
            send_event(event::convert(e));
        }

        void conversation_service::fire_quit_conversation_event(const std::string& id)
        {
            event::quit_conversation e{id};
            send_event(event::convert(e));
        }

        void conversation_service::fire_conversation_synced_event(const std::string& id)
        {
            event::conversation_synced e{id};
            send_event(event::convert(e));
        }

        void conversation_service::fire_contact_removed(
                const std::string& conversation_id, 
                const std::string& contact_id)
        {
            event::contact_removed e{conversation_id, contact_id};
            send_event(event::convert(e));
        }

        void conversation_service::fire_contact_added(
                const std::string& conversation_id, 
                const std::string& contact_id)
        {
            event::contact_added e{conversation_id, contact_id};
            send_event(event::convert(e));
        }

        void conversation_service::fire_conversation_alert(const std::string& id, bool visible)
        {
            event::conversation_alert e{id, visible};
            send_event(event::convert(e));
        }

    }
}

