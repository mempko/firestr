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
#ifndef FIRESTR_MESSAGE_MAILBOX_H
#define FIRESTR_MESSAGE_MAILBOX_H

#include <string>
#include <memory>

#include "message/message.hpp"
#include "util/queue.hpp"

namespace fire
{
    namespace message
    {
        class mailbox
        {
            public:
                mailbox();
                mailbox(const std::string&);

            public:
                const std::string& address() const;
                void address(const std::string&);

            public:
                void push_inbox(const message&);
                bool pop_inbox(message&);

            public:
                void push_outbox(const message&);
                bool pop_outbox(message&);

            private:
                typedef util::queue<message> message_queue;

                std::string _address;
                message_queue _in;
                message_queue _out;
        };

        typedef std::shared_ptr<mailbox> mailbox_ptr;
        typedef std::weak_ptr<mailbox> mailbox_wptr;
    }
}
#endif
