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

#ifndef FIRESTR_GUI_CONTACT_LIST_H
#define FIRESTR_GUI_CONTACT_LIST_H

#include "gui/list.hpp"
#include "user/user.hpp"
#include "user/userservice.hpp"

#include <QDialog>
#include <QPushButton>

namespace fire
{
    namespace gui
    {
        class user_info : public QWidget
        {
            Q_OBJECT
            public:
                user_info(user::user_info_ptr, user::user_service_ptr);

            public slots:
                void accept();
                void reject();

            private:
                user::user_info_ptr _contact;
                user::user_service_ptr _service;
                QPushButton* _accept;
                QPushButton* _reject;
        };

        class contact_list : public QDialog
        {
            Q_OBJECT
            public:
                contact_list(const std::string& title, user::user_service_ptr);
            public slots:
                void new_contact();
                void update();

            protected:
                void update_contacts();
            protected:
                list* _list;
                user::user_service_ptr _service;

                size_t _prev_requests;
                size_t _prev_contacts;
        };
    }
}

#endif
