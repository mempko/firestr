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

#ifndef FIRESTR_USER_USER_SERVICE_H
#define FIRESTR_USER_USER_SERVICE_H

#include "user/user.hpp"
#include "message/message.hpp"
#include "message/mailbox.hpp"
#include "util/thread.hpp"

#include <map>
#include <set>
#include <string>
#include <memory>

namespace fire
{
    namespace user
    {
        struct add_request
        {
            std::string to;
            std::string key;
            user_info_ptr from;
        };

        typedef std::map<std::string, add_request> add_requests;
        typedef std::set<std::string> sent_requests;

        class user_service
        {
            public:
                user_service(const std::string& home);
                ~user_service();

            public:
                local_user& user();
                const local_user& user() const;

            public:
                message::mailbox_ptr mail();

            public:
                void attempt_to_add_user(const std::string& address);
                void send_confirmation(const std::string& id, std::string key = "");
                void send_rejection(const std::string& id);

                const add_requests& pending_requests() const;

            private:
                void confirm_user(user_info_ptr contact);

            private:
                sent_requests _sent_requests;
                add_requests _pending_requests;

            private:
                local_user_ptr _user;
                message::mailbox_ptr _mail;
                std::string _home;
                bool _done;
                util::thread_uptr _thread;
                std::mutex _mutex;

            private:
                friend void user_service_thread(user_service*);
        };

        typedef std::shared_ptr<user_service> user_service_ptr;
        typedef std::weak_ptr<user_service> user_service_wptr;
    }
}

#endif
