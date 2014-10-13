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

        f_message(sync_conversation_msg)
        {
            std::string conversation_id;
            id_set contacts;
            id_set apps;

            f_message_init(sync_conversation_msg, SYNC_CONVERSATION);
            f_serialize
            {
                f_s(conversation_id);
                f_s(contacts);
                f_s(apps);
            }
            
        };

        f_message(quit_conversation_msg)
        {
            std::string conversation_id;

            f_message_init(quit_conversation_msg, QUIT_CONVERSATION);
            f_serialize
            {
                f_s(conversation_id);
            }
        };

        f_message(ask_contact_req_msg)
        {
            std::string conversation_id;
            std::string id;

            f_message_init(ask_contact_req_msg, ASK_CONTACT_REQ);
            f_serialize
            {
                f_s(conversation_id);
                f_s(id);
            }
        };

        f_message(ask_contact_res_msg)
        {
            std::string id;
            std::string conversation_id;
            int status;

            f_message_init(ask_contact_res_msg, ASK_CONTACT_RES);
            f_serialize
            {
                f_s(id);
                f_s(conversation_id);
                f_s(status);
            }
        };

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

            init_handlers();
            start();

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

        void conversation_service::init_handlers()
        {
            using std::bind;
            using namespace std::placeholders;

            handle(SYNC_CONVERSATION, bind(&conversation_service::received_sync, this, _1));
            handle(QUIT_CONVERSATION, bind(&conversation_service::received_quit, this, _1));
            handle(ASK_CONTACT_REQ, bind(&conversation_service::received_ask_contact_req, this, _1));
            handle(ASK_CONTACT_RES, bind(&conversation_service::received_ask_contact_res, this, _1));
        }

        void conversation_service::received_sync(const message::message& m)
        {
            INVARIANT(_user_service);
            REQUIRE_EQUAL(m.meta.type, SYNC_CONVERSATION);
            m::expect_symmetric(m);

            sync_conversation_msg s;
            s.from_message(m);

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

        void conversation_service::received_quit(const message::message& m)
        {
            INVARIANT(_user_service);
            REQUIRE_EQUAL(m.meta.type, QUIT_CONVERSATION);
            m::expect_symmetric(m);

            quit_conversation_msg q;
            q.from_message(m);

            auto c = _user_service->user().contacts().by_id(q.from_id);
            if(!c) return;

            auto s = conversation_by_id(q.conversation_id);
            if(!s) return;

            c = s->contacts().by_id(q.from_id);
            if(!c) return;

            s->contacts().remove(c);
            fire_contact_removed(q.conversation_id, q.from_id);
        }

        void conversation_service::received_ask_contact_req(const message::message& m)
        {
            INVARIANT(_user_service);
            REQUIRE_EQUAL(m.meta.type, ASK_CONTACT_REQ);
            m::expect_symmetric(m);

            ask_contact_req_msg a;
            a.from_message(m);

            bool hc = _user_service->user().contacts().has(a.from_id);
            if(!hc) return;

            auto s = conversation_by_id(a.conversation_id);
            if(!s) return;

            bool know = _user_service->user().contacts().has(a.id);
            ask_contact_res_msg r;
            r.conversation_id = a.conversation_id;
            r.id = a.id;
            r.status = know ? 1 : 0;
            _sender->send(a.from_id, r.to_message());
        }

        void conversation_service::received_ask_contact_res(const message::message& m)
        {
            INVARIANT(_user_service);
            REQUIRE_EQUAL(m.meta.type, ASK_CONTACT_RES);
            m::expect_symmetric(m);

            ask_contact_res_msg r;
            r.from_message(m);

            auto fc = _user_service->user().contacts().by_id(r.from_id);
            if(!fc) return;

            auto c = _user_service->user().contacts().by_id(r.id);
            if(!c) return;

            auto s = conversation_by_id(r.conversation_id);
            if(!s) return;

            s->know_contact(r.id, r.from_id, 
                    r.status == 1 ? 
                    know_request::KNOW : know_request::DONT_KNOW);

            if(s->part_of_clique(r.id))
                add_contact_to_conversation_p(c, s);
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
            auto m = ns.to_message();

            for(auto c : s->second->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), m);
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
            auto m = ns.to_message();

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), m);
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

            ask_contact_req_msg r;
            r.id = id;
            r.conversation_id = s->id();
            auto m = r.to_message();

            for(auto c : s->contacts().list())
            {
                CHECK(c);
                _sender->send(c->id(), m);
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
        }

        void conversation_service::fire_new_conversation_event(const std::string& id)
        {
            event::new_conversation e;
            e.conversation_id = id;
            send_event(e.to_message());
        }

        void conversation_service::fire_quit_conversation_event(const std::string& id)
        {
            event::quit_conversation e;
            e.conversation_id = id;
            send_event(e.to_message());
        }

        void conversation_service::fire_conversation_synced_event(const std::string& id)
        {
            event::conversation_synced e;
            e.conversation_id = id;
            send_event(e.to_message());
        }

        void conversation_service::fire_contact_removed(
                const std::string& conversation_id, 
                const std::string& contact_id)
        {
            event::contact_removed e;
            e.conversation_id = conversation_id;
            e.contact_id = contact_id;
            send_event(e.to_message());
        }

        void conversation_service::fire_contact_added(
                const std::string& conversation_id, 
                const std::string& contact_id)
        {
            event::contact_added e;
            e.conversation_id = conversation_id;
            e.contact_id = contact_id;
            send_event(e.to_message());
        }

        void conversation_service::fire_conversation_alert(const std::string& id, bool visible)
        {
            event::conversation_alert e;
            e.conversation_id = id;
            e.visible = visible;
            send_event(e.to_message());
        }

    }
}

