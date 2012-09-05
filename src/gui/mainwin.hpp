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

#include "gui/session.hpp"
#include "message/postoffice.hpp"
#include "user/userservice.hpp"
#include "session/session.hpp"
#include "session/session_service.hpp"

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
                        const std::string& ping,
                        const std::string& home);

            private slots:
                void about();
                void show_contact_list();
                void show_contact_list_start();
                void make_test_message();
                void closeEvent(QCloseEvent*);
                void check_mail();
                void create_session();
                void rename_session();

            private:
                void setup_post(
                        const std::string& host, 
                        const std::string& port);
                void create_actions();
                void create_main();
                void create_menus();
                void make_new_user();
                void setup_services(const std::string& ping);
                void save_state();
                void restore_state();
                void setup_timers();
                void create_start_screen();

            private:
                //gui message handlers

                void new_session(const std::string& id);

            private:
                QMenu *_main_menu;
                QMenu *_contact_menu;
                QAction *_close_action;
                QAction *_about_action;
                QAction *_contact_list_action;
                QMenu *_session_menu;
                QAction *_create_session_action;
                QAction *_rename_session_action;
                QMenu *_test_menu;
                QAction *_test_message_action;

                QTabWidget* _sessions;
                QWidget* _start_screen;

                QWidget* _root;
                QVBoxLayout* _layout;

            private:
                fire::message::post_office_ptr _master;
                fire::message::mailbox_ptr _mail;
                user::user_service_ptr _user_service;
                session::session_service_ptr _session_service;
                std::string _home;
        };
    }
}

#endif
