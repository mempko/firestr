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
        using queue = util::queue<message>;

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

            public:
                size_t in_size() const;
                size_t out_size() const;

            private:

                std::string _address;
                queue _in;
                queue _out;
        };

        using mailbox_ptr = std::shared_ptr<mailbox>;
        using mailbox_wptr = std::weak_ptr<mailbox>;
    }
}
#endif
