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

#ifndef FIRESTR_GUI_CONTACT_SELECT_H
#define FIRESTR_GUI_CONTACT_SELECT_H

#include "user/user_service.hpp"

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
