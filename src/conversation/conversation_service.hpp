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
 */

#ifndef FIRESTR_CONVERSATION_CONVERSATION_SERVICE_H
#define FIRESTR_CONVERSATION_CONVERSATION_SERVICE_H

#include "conversation/conversation.hpp"
#include "service/service.hpp"
#include "message/postoffice.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <map>
#include <mutex>

namespace fire
{
    namespace conversation
    {
        using conversation_map = std::map<std::string, conversation_ptr>;
        using app_addresses = std::set<std::string>;
        class conversation_service : public service::service 
        {
            public:
                conversation_service(
                        message::post_office_ptr,
                        user::user_service_ptr,
                        message::mailbox_ptr event = nullptr);
                ~conversation_service();

            public:
                conversation_ptr sync_conversation(
                        const std::string& from_id, //may be empty 
                        const std::string& id, 
                        const user::contact_list&, 
                        const app_addresses&);

                conversation_ptr create_conversation(const std::string& id);
                conversation_ptr create_conversation(user::contact_list&);
                conversation_ptr create_conversation();
                void quit_conversation(const std::string& id);

            public:
                conversation_ptr conversation_by_id(const std::string&);

                bool add_contact_to_conversation( 
                        const user::user_info_ptr contact, 
                        const std::string& conversation_id);

                void add_contact_to_conversation( 
                        const user::user_info_ptr contact, 
                        conversation_ptr conversation);

                void sync_existing_conversation(conversation_ptr conversation);
                void sync_existing_conversation(const std::string& conversation_id);
                void broadcast_message(const message::message&);

            public:
                user::user_service_ptr user_service();

            public:
                void fire_conversation_alert(const std::string& id);

            protected:
                virtual void message_received(const message::message&);

            private:
                void fire_new_conversation_event(const std::string& id);
                void fire_quit_conversation_event(const std::string& id);
                void fire_conversation_synced_event(const std::string& id);
                void fire_contact_removed(
                        const std::string& conversation_id,
                        const std::string& contact_id);
                void fire_contact_added(
                        const std::string& conversation_id,
                        const std::string& contact_id);

                void request_apps(
                        const std::string& from_id,
                        conversation_ptr s, 
                        const app_addresses&);

            private:
                message::post_office_ptr _post;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                conversation_map _conversations;
                std::mutex _mutex;
        };

        using conversation_service_ptr = std::shared_ptr<conversation_service>;
        using conversation_servie_wptr = std::weak_ptr<conversation_service>;

        //events
        namespace event
        {
            extern const std::string NEW_CONVERSATION;
            struct new_conversation
            {
                std::string conversation_id;
            };
            message::message convert(const new_conversation&);
            void convert(const message::message&, new_conversation&);

            extern const std::string QUIT_CONVERSATION;
            struct quit_conversation
            {
                std::string conversation_id;
            };
            message::message convert(const quit_conversation&);
            void convert(const message::message&, quit_conversation&);


            extern const std::string CONVERSATION_SYNCED;
            struct conversation_synced
            {
                std::string conversation_id;
            };
            message::message convert(const conversation_synced&);
            void convert(const message::message&, conversation_synced&);

            extern const std::string CONTACT_REMOVED;
            struct contact_removed
            {
                std::string conversation_id;
                std::string contact_id;
            };
            message::message convert(const contact_removed&);
            void convert(const message::message&, contact_removed&);

            extern const std::string CONTACT_ADDED;
            struct contact_added
            {
                std::string conversation_id;
                std::string contact_id;
            };
            message::message convert(const contact_added&);
            void convert(const message::message&, contact_added&);

            extern const std::string CONVERSATION_ALERT;
            struct conversation_alert
            {
                std::string conversation_id;
            };
            message::message convert(const conversation_alert&);
            void convert(const message::message&, conversation_alert&);
        }
    }
}

#endif

