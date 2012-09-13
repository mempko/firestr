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
#include "gui/app/app_service.hpp"

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
                void make_chat_sample();
                void make_script_sample();
                void closeEvent(QCloseEvent*);
                void check_mail();
                void create_session();
                void create_session(QString id);
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
                void attach_start_screen();
                void create_alert_screen();
                void show_alert(QWidget*);

            private:
                //gui service event handlers

                void new_session_event(const std::string& id);
                void new_contact_event(const std::string& id);
                void contact_connected_event(const std::string& id);
                void contact_disconnected_event(const std::string& id);

            private:
                QMenu *_main_menu;
                QMenu *_contact_menu;
                QAction *_close_action;
                QAction *_about_action;
                QAction *_contact_list_action;
                QMenu *_session_menu;
                QAction *_create_session_action;
                QAction *_rename_session_action;
                QMenu *_app_menu;
                QAction *_chat_sample_action;
                QAction *_script_sample_action;

                QTabWidget* _sessions;
                QWidget* _start_screen;
                bool _start_screen_attached;

                QWidget* _alert_screen;
                list* _alerts;

                QWidget* _root;
                QVBoxLayout* _layout;

            private:
                fire::message::post_office_ptr _master;
                fire::message::mailbox_ptr _mail;
                user::user_service_ptr _user_service;
                session::session_service_ptr _session_service;
                app::app_service_ptr _app_service;
                std::string _home;
        };
    }
}

#endif
