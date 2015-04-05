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
                mailboxes boxes() const;


            public:
                bool add(post_office_wptr);
                bool has(post_office_wptr) const;
                void remove_post_office(const std::string&);
                const post_offices& offices() const;

            public:
                post_office* parent() ;
                const post_office* parent() const;
                void parent(post_office*); 

            public:
                const mailbox_stats& outside_stats() const;
                mailbox_stats& outside_stats();
                void outside_stats(bool);

            protected:
                void clean_mailboxes();

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
                mailbox_stats _outside_stats;

            protected:
                friend void send_thread(post_office*);
        };
    }
}

#endif
