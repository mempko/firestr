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

#ifndef FIRESTR_SESSION_SESSION_H
#define FIRESTR_SESSION_SESSION_H

#include "user/user.hpp"
#include "user/userservice.hpp"

#include "message/postoffice.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fire
{
    namespace session
    {
        using app_mailbox_ids = std::vector<std::string>;
        class session 
        {
            public:
                session(user::user_service_ptr, message::post_office_wptr);
                session(const std::string id, user::user_service_ptr, message::post_office_wptr);

            public:
                const user::contact_list& contacts() const;
                user::contact_list& contacts(); 

            public:
                bool send(const std::string& to, const message::message& m);
                bool send(const message::message&);

            public:
                const std::string& id() const;
                bool initiated_by_user() const;
                void initiated_by_user(bool);

            public:
                message::post_office_wptr parent_post();
                message::mailbox_ptr mail();
                messages::sender_ptr sender();
                user::user_service_ptr user_service();

            public:
                const app_mailbox_ids& app_ids() const;
                void add_app_id(const std::string& id);

            private:
                void init();

            private:
                std::string _id;
                message::post_office_wptr _parent_post;
                message::mailbox_ptr _mail;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                user::contact_list _contacts;
                app_mailbox_ids _app_mailbox_ids;
                bool _initiated_by_user;
        };

        using session_ptr = std::shared_ptr<session>;
        using session_wptr = std::weak_ptr<session>;
    }
}

#endif

