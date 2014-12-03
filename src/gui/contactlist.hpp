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

#ifndef FIRESTR_GUI_CONTACT_LIST_H
#define FIRESTR_GUI_CONTACT_LIST_H

#include "gui/list.hpp"
#include "gui/contactselect.hpp"
#include "user/user.hpp"
#include "user/userservice.hpp"

#include <set>
#include <string>
#include <functional>

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
                        bool compact = true,
                        QPushButton* action = nullptr);

            public slots:
                void update(bool update_action = false);
                void update(std::function<bool(user::user_info&)> f, bool update_action = false);

            private:
                user::user_info_ptr _contact;
                user::user_service_ptr _service;
                QPushButton* _action;
                QLabel* _user_text;
        };
        using user_info_ptrs = std::vector<user_info*>;


        using make_user_info = std::function<user_info*(user::user_info_ptr)>;

        class contact_list : public list
        {
            Q_OBJECT
            public:
                contact_list(user::user_service_ptr, const user::contact_list&);
                contact_list(user::user_service_ptr, const user::contact_list&, make_user_info);

            public slots:
                void add_contact(user::user_info_ptr);
                void update(const user::contact_list&);
                void update_status(bool update_action = false);
                void update_status(std::function<bool(user::user_info&)> f, bool update_action = false);

            protected:
                user::user_service_ptr _service;
                user::contact_list _contacts;
                user_info_ptrs _contact_widgets;
                make_user_info _mk;
        };

        class greeter_info : public QWidget
        {
            Q_OBJECT
            public:
                greeter_info(user::user_service_ptr, const user::greet_server& );
            private slots:
                void remove();

            private:
                QLabel* _label;
                QPushButton* _rm;

            private:
                std::string _address;
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

            protected:
                user::user_service_ptr _service;
        };

        class intro_info : public QWidget
        {
            Q_OBJECT
            public:
                intro_info(user::user_service_ptr, const user::contact_introduction&);
            private slots:
                void remove();
                void accept();

            private:
                QLabel* _label;
                QLabel* _message;
                QPushButton* _accept;
                QPushButton* _rm;

            private:
                user::contact_introduction _intro;
                user::user_service_ptr _service;

        };

        class intro_list : public list
        {
            Q_OBJECT
            public:
                intro_list(user::user_service_ptr);

            public slots:
                void introduce();

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
                        QWidget* parent = nullptr);

            public:
                void save_state();
#ifdef _WIN64
                bool new_contact(const unsigned short*);
#else
                bool new_contact(const std::string&);
#endif

            public slots:
                void new_contact();
                void update();
                void remove(QString);

            protected:
                void init_contacts_tab(QWidget* tab, QGridLayout* layout);
                void init_intro_tab(QWidget* tab, QGridLayout* layout);
                void init_greeters_tab(QWidget* tab, QGridLayout* layout);
                void update_contacts();
                void restore_state();

            protected:
                list* _list;
                user::user_service_ptr _service;
                greeter_list* _greeters;

                size_t _prev_contacts;
        };

        class introduce_dialog : public QDialog
        {
            Q_OBJECT
            public:
                introduce_dialog(
                        const std::string& title, 
                        user::user_service_ptr, 
                        QWidget* parent = nullptr);
            public slots:
                void introduce();
                void contact_1_selected(int);
                void contact_2_selected(int);

            protected:
                void update_widgets();

            protected:
                user::user_service_ptr _user_service;

                contact_select_widget* _contact_1;
                contact_select_widget* _contact_2;

                QLabel* _message_label_1;
                QLabel* _message_label_2;
                QLineEdit* _message_1;
                QLineEdit* _message_2;
                QPushButton* _introduce;
        };

#ifdef _WIN64
        bool add_contact_gui(user::user_service_ptr, const unsigned short*, QWidget*);
#else
        bool add_contact_gui(user::user_service_ptr, const std::string&, QWidget*);
#endif

        void add_contact_gui(user::user_service_ptr, QWidget*);
        void create_contact_file(user::user_service_ptr, QWidget* parent);
    }
}

#endif
