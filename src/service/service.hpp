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
 */

#ifndef FIRESTR_SERVICE_SERVICE_H
#define FIRESTR_SERVICE_SERVICE_H

#include "message/message.hpp"
#include "message/mailbox.hpp"
#include "util/thread.hpp"

#include <string>
#include <memory>

namespace fire
{
    namespace service
    {
        class service
        {
            public:
                service(
                        const std::string& address,
                        message::mailbox_ptr event = nullptr);
                virtual ~service();

            public:
                message::mailbox_ptr mail();

            protected:
                virtual void message_received(const message::message&) = 0;

            protected:
                void send_event(const message::message&);

            private:
                std::string _address;
                bool _done;
                message::mailbox_ptr _mail;
                util::thread_uptr _thread;
                message::mailbox_ptr _event;

            private:
                friend void service_thread(service*);
        };

        using service_ptr = std::shared_ptr<service>;
        using service_wptr = std::weak_ptr<service>;
    }
}
#endif
