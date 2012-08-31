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

#ifndef FIRESTR_MAINWIN_H
#define FIRESTR_MAINWIN_H

#include <QMainWindow>

#include "gui/messagelist.hpp"
#include "message/postoffice.hpp"
#include "user/userservice.hpp"
#include "session/session.hpp"

namespace fire
{
    namespace gui
    {
        user::local_user_ptr setup_user(const std::string& home);

        class main_window : public QMainWindow
        {
            Q_OBJECT
            public:
                main_window(
                        const std::string& host, 
                        const std::string& port,
                        const std::string& home);

            private slots:
                void about();
                void show_contact_list();
                void make_test_message();

            private:
                void setup_post(
                        const std::string& host, 
                        const std::string& port);
                void create_actions();
                void create_main();
                void create_menus();
                void make_new_user();
                void setup_services();

            private:
                QMenu *_main_menu;
                QMenu *_contact_menu;
                QAction *_close_action;
                QAction *_about_action;
                QAction *_contact_list_action;
                QMenu *_test_menu;
                QAction *_test_message_action;

                QWidget* _root;
                QVBoxLayout* _layout;

                message_list* _messages;
                fire::message::post_office_ptr _master;
                user::user_service_ptr _user_service;
                session::session_ptr _session;
                std::string _home;
        };
    }
}

#endif
