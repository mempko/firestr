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

#ifndef FIRESTR_SERVICE_SERVICE_H
#define FIRESTR_SERVICE_SERVICE_H

#include "message/message.hpp"
#include "message/mailbox.hpp"
#include "util/thread.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace fire
{
    namespace service
    {
        using message_handler = std::function<void (const message::message&)>;
        using handler_map = std::unordered_map<std::string, message_handler>;

        class service_map
        {
            public:
                void handle(const std::string& type, message_handler);
                bool handle(const message::message& m);
                size_t total_handlers() const;

            private:
                handler_map _h;
        };

        class service
        {
            public:
                service(
                        const std::string& address,
                        message::mailbox_ptr event = nullptr);
                service(
                        message::mailbox_ptr mail,
                        message::mailbox_ptr event = nullptr);
                virtual ~service();

            public:
                void start();
                void stop();
                message::mailbox_ptr mail();

            public:
                void handle(const std::string& type, message_handler);

            protected:
                void send_event(const message::message&);

            private:
                void message_received(const message::message&);
                std::string _address;
                bool _done;
                message::mailbox_ptr _mail;
                util::thread_uptr _thread;
                message::mailbox_ptr _event;

            private:
                friend void service_thread(service*);
                service_map _sm;
        };

        using service_ptr = std::shared_ptr<service>;
        using service_wptr = std::weak_ptr<service>;
    }
}
#endif
