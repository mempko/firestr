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

#ifndef FIRESTR_GUI_MESSAGELIST_H
#define FIRESTR_GUI_MESSAGELIST_H

#include <QScrollArea>


#ifndef Q_MOC_RUN
#include "gui/list.hpp"
#include "gui/message.hpp"
#include "gui/app/app_service.hpp"
#include "gui/contactlist.hpp"
#include "session/session.hpp"
#include "messages/new_app.hpp"
#endif

namespace fire
{
    namespace gui
    {
        class message_list : public list
        {
            Q_OBJECT
            public:
                message_list(
                        app::app_service_ptr,
                        session::session_ptr);

            public:
                session::session_ptr session();
                app::app_service_ptr app_service();

            public:
                void update_contact_lists();
                void remove_from_contact_lists(user::user_info_ptr);

            public slots:
                std::string add_new_app(const messages::new_app&); 
                void add(message*);
                void add(QWidget*);

            private:
                session::session_ptr _session;
                app::app_service_ptr _app_service;

                using contact_list_ptrs = std::vector<contact_list*>;
                using message_contacts = std::vector<user::contact_list>;
                contact_list_ptrs _contact_lists;
                message_contacts _message_contacts;
        };
    }
}

#endif
