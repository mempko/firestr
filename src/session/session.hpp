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

#ifndef FIRESTR_SESSION_SESSION_H
#define FIRESTR_SESSION_SESSION_H

#include "user/user.hpp"
#include "user/userservice.hpp"

#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <memory>
#include <string>

namespace fire
{
    namespace session
    {
        class session 
        {
            public:
                session(user::user_service_ptr);
                session(const std::string id, user::user_service_ptr);

            public:
                const user::contact_list& contacts() const;
                user::contact_list& contacts(); 

            public:
                const std::string& id() const;

            public:
                message::mailbox_ptr mail();
                messages::sender_ptr sender();
                user::user_service_ptr user_service();

            private:
                void init();

            private:
                std::string _id;
                message::mailbox_ptr _mail;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                user::contact_list _contacts;
        };

        typedef std::shared_ptr<session> session_ptr;
        typedef std::weak_ptr<session> session_wptr;
    }
}

#endif

