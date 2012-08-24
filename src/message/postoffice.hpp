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
#ifndef FIRESTR_MESSAGE_POSTOFFICE_H
#define FIRESTR_MESSAGE_POSTOFFICE_H

#include <string>
#include <map>
#include <memory>
#include <thread>

#include "message/mailbox.hpp"

namespace fire
{
    namespace message
    {
        class post_office;
        typedef std::shared_ptr<post_office> post_office_ptr;
        typedef std::weak_ptr<post_office> post_office_wptr;
        typedef std::unique_ptr<std::thread> thread_uptr;

        class post_office
        {
            public:
                post_office();
                post_office(const std::string&);
                ~post_office();

            public:
                const std::string& address() const;
                void address(const std::string&);

            public:
                bool send(message);

            public:
                bool add(mailbox_wptr);
                void remove_mailbox(const std::string&);

            public:
                bool add(post_office_wptr);
                void remove_post_office(const std::string&);

            public:
                post_office* parent() ;
                const post_office* parent() const;
                void parent(post_office*); 

            private:
                bool send_outside(const message&);

            private:
                typedef std::map<std::string, mailbox_wptr> mailboxes;
                typedef std::map<std::string, post_office_wptr> post_offices;

                std::string _address;
                mailboxes _boxes;
                post_offices _offices;
                post_office* _parent;
                thread_uptr _send_thread;
                bool _done;

            private:
                friend void send_thread(post_office*);
        };
    }
}

#endif
