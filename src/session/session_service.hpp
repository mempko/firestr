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

#ifndef FIRESTR_SESSION_SESSION_SERVICE_H
#define FIRESTR_SESSION_SESSION_SERVICE_H

#include "session/session.hpp"
#include "service/service.hpp"
#include "message/postoffice.hpp"
#include "messages/sender.hpp"

#include <map>

namespace fire
{
    namespace session
    {
        typedef std::map<std::string, session_ptr> session_map;
        class session_service : public service::service 
        {
            public:
                session_service(
                        message::post_office_ptr,
                        user::user_service_ptr);

            public:
                session_ptr create_session(const std::string& id);
                session_ptr create_session(const std::string& id, const user::users&);
                session_ptr create_session(user::users&);
                session_ptr create_session();

            public:
                session_ptr session_by_id(const std::string&);
                bool add_contact_to_session( 
                        const user::user_info_ptr contact, 
                        const std::string& session_id);

            protected:
                virtual void message_recieved(const message::message&);

            private:
                message::post_office_ptr _post;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                session_map _sessions;
        };

        typedef std::shared_ptr<session_service> session_service_ptr;
        typedef std::weak_ptr<session_service> session_servie_wptr;
    }
}

#endif

