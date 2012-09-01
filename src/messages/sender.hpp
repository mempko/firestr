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
#ifndef FIRESTR_MESSAGES_SENDER_H
#define FIRESTR_MESSAGES_SENDER_H

#include "message/message.hpp"
#include "user/userservice.hpp"

#include <string>
#include <memory>

namespace fire
{
    namespace messages
    {
        class sender
        {
            public:
                sender(user::user_service_ptr, message::mailbox_ptr);
            public:

                /**
                 * Send a message to the recipient
                 * @param to Id of user
                 */
                bool send(const std::string& to, message::message);

            public:
                user::user_service_ptr user_service();

            private:
                user::user_service_ptr _service;
                message::mailbox_ptr _mail;
        };

        typedef std::shared_ptr<sender> sender_ptr;
        typedef std::weak_ptr<sender> sender_wptr;
    }
}
#endif
