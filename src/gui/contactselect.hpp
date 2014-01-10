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

#ifndef FIRESTR_GUI_CONTACT_SELECT_H
#define FIRESTR_GUI_CONTACT_SELECT_H

#include "user/userservice.hpp"

#include <QWidget>
#include <QGridLayout>
#include <QComboBox>

#include <functional>

namespace fire
{
    namespace gui
    {
        using contact_select_filter = std::function<bool (const user::user_info&)>;
        inline bool identity_filter(const user::user_info&) { return true;}

        class contact_select_widget : public QComboBox
        {
            Q_OBJECT
            public:
                contact_select_widget(
                        user::user_service_ptr, 
                        contact_select_filter f = identity_filter);

            public:
                user::user_service_ptr user_service();
                user::user_info_ptr selected_contact();

            public slots:
                void update_contacts();

            private:
                user::user_service_ptr _user_service;
                contact_select_filter _filter;
        };
    }
}

#endif
