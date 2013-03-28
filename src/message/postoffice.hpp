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
#ifndef FIRESTR_MESSAGE_POSTOFFICE_H
#define FIRESTR_MESSAGE_POSTOFFICE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <thread>

#include "message/mailbox.hpp"
#include "util/thread.hpp"

namespace fire
{
    namespace message
    {
        class post_office;
        using post_office_ptr = std::shared_ptr<post_office>;
        using post_office_wptr = std::weak_ptr<post_office>;
        using thread_uptr = std::unique_ptr<std::thread>;
        using mailboxes = std::unordered_map<std::string, mailbox_wptr>;
        using post_offices = std::unordered_map<std::string, post_office_wptr>;

        class post_office
        {
            public:
                post_office();
                post_office(const std::string&);
                virtual ~post_office();

            public:
                const std::string& address() const;
                void address(const std::string&);

            public:
                bool send(message);

            public:
                bool add(mailbox_wptr);
                bool has(mailbox_wptr) const;
                void remove_mailbox(const std::string&);
                const mailboxes& boxes() const;


            public:
                bool add(post_office_wptr);
                bool has(post_office_wptr) const;
                void remove_post_office(const std::string&);
                const post_offices& offices() const;

            public:
                post_office* parent() ;
                const post_office* parent() const;
                void parent(post_office*); 

            protected:
                virtual bool send_outside(const message&);

            protected:

                std::string _address;
                mailboxes _boxes;
                post_offices _offices;
                post_office* _parent;
                util::thread_uptr _send_thread;
                bool _done;
                mutable std::mutex _box_m;
                mutable std::mutex _post_m;

            protected:
                friend void send_thread(post_office*);
        };
    }
}

#endif
