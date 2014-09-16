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

#include "messages/sender.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace messages
    {
        sender::sender(user::user_service_ptr service, message::mailbox_ptr mail) :
            _service{service},
            _mail{mail}
        {
            REQUIRE(service);
            REQUIRE(mail);

            INVARIANT(_service);
            INVARIANT(_mail);
        }

        bool sender::send(const std::string& to, message::message m)
        {
            INVARIANT(_service);
            INVARIANT(_mail);

            auto contact = _service->user().contacts().by_id(to);
            if(!contact) return false;

            const auto my_id = _service->user().info().id();
            m.meta.to = {contact->address(), _mail->address()};
            m.meta.extra["from_id"] = my_id;

            _mail->push_outbox(m);
            return true;
        }

        bool sender::send_to_local_app(const std::string& address, message::message m)
        {
            INVARIANT(_service);
            INVARIANT(_mail);

            const auto& my_id = _service->user().info().id();

            m.meta.to = {address};
            m.meta.extra["from_id"] = my_id;
            m.meta.extra["local_app_id"] = _mail->address();

            _mail->push_outbox(m);

            return true;
        }

        user::user_service_ptr sender::user_service()
        {
            ENSURE(_service);
            return _service;
        }
    }
}
