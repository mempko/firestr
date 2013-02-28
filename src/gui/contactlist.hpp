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

#include <set>
#include <string>

#include <QDialog>
#include <QPushButton>
#include <QLabel>

namespace fire
{
    namespace gui
    {
        class user_info : public QWidget
        {
            Q_OBJECT
            public:
                user_info(
                        user::user_info_ptr, 
                        user::user_service_ptr,
                        bool accept_reject = false,
                        bool compact = false);

            public slots:
                void accept();
                void reject();
                void update();

            private:
                user::user_info_ptr _contact;
                user::user_service_ptr _service;
                QPushButton* _accept;
                QPushButton* _reject;
                QLabel* _online;
        };
        using user_info_ptrs = std::vector<user_info*>;

        class contact_list : public list
        {
            Q_OBJECT
            public:
                contact_list(user::user_service_ptr, const user::contact_list&);

            public slots:
                void add_contact(user::user_info_ptr);
                void update(const user::contact_list&);
                void update_status();

            protected:
                user::user_service_ptr _service;
                user::contact_list _contacts;
                user_info_ptrs _contact_widgets;
        };

        class greeter_info : public QWidget
        {
            Q_OBJECT
            public:
                greeter_info(user::user_service_ptr, const user::greet_server& );

            private:
                user::greet_server _server;
                user::user_service_ptr _service;
        };

        class greeter_list : public list
        {
            Q_OBJECT
            public:
                greeter_list(user::user_service_ptr);
                void add_greeter(const user::greet_server& s);

            public slots:
                void add_greeter();
                void remove_greeter();

            protected:
                user::user_service_ptr _service;
        };

        class contact_list_dialog : public QDialog
        {
            Q_OBJECT
            public:
                contact_list_dialog(
                        const std::string& title, 
                        user::user_service_ptr, 
                        bool add_on_start = false);

            public slots:
                void new_contact();
                void update();

            protected:
                void init_contacts_tab(QWidget* tab, QGridLayout* layout, bool add_on_start);
                void init_greeters_tab(QWidget* tab, QGridLayout* layout);
                void update_contacts();

            protected:
                list* _list;
                user::user_service_ptr _service;
                greeter_list* _greeters;

                size_t _prev_requests;
                size_t _prev_contacts;
        };

        class add_contact_dialog : public QDialog
        {
            Q_OBJECT
            public:
                add_contact_dialog(user::user_service_ptr, QWidget* parent = nullptr);

            public slots:
                void new_local_contact();
                void new_remote_contact();
                void create_contact_file();

            private:
                user::user_service_ptr _service;
        };
    }
}

#endif
