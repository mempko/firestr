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

#ifndef FIRESTR_SESSION_SESSION_SERVICE_H
#define FIRESTR_SESSION_SESSION_SERVICE_H

#include "session/session.hpp"
#include "service/service.hpp"
#include "message/postoffice.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <map>
#include <mutex>

namespace fire
{
    namespace session
    {
        using session_map = std::map<std::string, session_ptr>;
        class session_service : public service::service 
        {
            public:
                session_service(
                        message::post_office_ptr,
                        user::user_service_ptr,
                        message::mailbox_ptr event = nullptr);

            public:
                session_ptr sync_session(const std::string& id, const user::contact_list&);
                session_ptr create_session(const std::string& id);
                session_ptr create_session(user::contact_list&);
                session_ptr create_session();
                void quit_session(const std::string& id);

            public:
                session_ptr session_by_id(const std::string&);

                bool add_contact_to_session( 
                        const user::user_info_ptr contact, 
                        const std::string& session_id);

                void add_contact_to_session( 
                        const user::user_info_ptr contact, 
                        session_ptr session);

                void sync_existing_session(session_ptr session);
                void sync_existing_session(const std::string& session_id);
                void broadcast_message(const message::message&);

            public:
                user::user_service_ptr user_service();

            protected:
                virtual void message_recieved(const message::message&);

            private:
                void fire_new_session_event(const std::string& id);
                void fire_quit_session_event(const std::string& id);
                void fire_session_synced_event(const std::string& id);
                void fire_contact_removed(
                        const std::string& session_id,
                        const std::string& contact_id);
                void fire_contact_added(
                        const std::string& session_id,
                        const std::string& contact_id);

            private:
                message::post_office_ptr _post;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                session_map _sessions;
                std::mutex _mutex;
        };

        using session_service_ptr = std::shared_ptr<session_service>;
        using session_servie_wptr = std::weak_ptr<session_service>;

        //events
        namespace event
        {
            extern const std::string NEW_SESSION;
            struct new_session
            {
                std::string session_id;
            };
            message::message convert(const new_session&);
            void convert(const message::message&, new_session&);

            extern const std::string QUIT_SESSION;
            struct quit_session
            {
                std::string session_id;
            };
            message::message convert(const quit_session&);
            void convert(const message::message&, quit_session&);


            extern const std::string SESSION_SYNCED;
            struct session_synced
            {
                std::string session_id;
            };
            message::message convert(const session_synced&);
            void convert(const message::message&, session_synced&);

            extern const std::string CONTACT_REMOVED;
            struct contact_removed
            {
                std::string session_id;
                std::string contact_id;
            };
            message::message convert(const contact_removed&);
            void convert(const message::message&, contact_removed&);

            extern const std::string CONTACT_ADDED;
            struct contact_added
            {
                std::string session_id;
                std::string contact_id;
            };
            message::message convert(const contact_added&);
            void convert(const message::message&, contact_added&);
        }
    }
}

#endif

