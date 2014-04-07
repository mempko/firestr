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

#ifndef FIRESTR_MAINWIN_H
#define FIRESTR_MAINWIN_H

#include <QMainWindow>

#ifndef Q_MOC_RUN
#include "gui/conversation.hpp"
#include "gui/app/app_service.hpp"
#include "gui/mail_service.hpp"
#include "message/postoffice.hpp"
#include "user/userservice.hpp"
#include "conversation/conversation.hpp"
#include "conversation/conversation_service.hpp"
#include "security/security_library.hpp"
#include "util/dbc.hpp"
#endif

namespace fire
{
    namespace gui
    {
        user::local_user_ptr setup_user(const std::string& home);

        class MainTabs : public QTabWidget
        {
            Q_OBJECT
            public:
                void setTabTextColor(int i, const QColor& c) 
                { 
                    INVARIANT(tabBar());
                    tabBar()->setTabTextColor(i, c);
                }
        };

        struct main_window_context
        {
            std::string home;
            std::string host;
            network::port_type port;
            user::local_user_ptr user;
            bool debug;
        };

        class main_window : public QMainWindow
        {
            Q_OBJECT
            public:
                main_window(const main_window_context&);
                ~main_window();

            private slots:
                void about();
                void show_contact_list();
                void show_contact_list_start();
                void make_chat_app();
                void make_app_editor();
                void closeEvent(QCloseEvent*);
                void check_mail(fire::message::message);
                void create_conversation();
                void create_conversation(QString id);
                void rename_conversation();
                void quit_conversation();
                void tab_changed(int);
                void load_app_into_conversation(QString id);
                void show_debug_window();
                void remove_alert(QWidget*);
                void focus_changed(QWidget* old, QWidget* now);

            private:
                void setup_post();
                void create_actions();
                void create_main();
                void create_menus();
                void create_app_menu();
                void update_app_menu();
                void make_new_user();
                void setup_services();
                void save_state();
                void restore_state();
                void setup_timers();
                void create_start_screen();
                void attach_start_screen();
                void create_alert_screen();
                void show_alert(QWidget*);
                bool should_alert(int tab_index);
                void alert_tab(int tab_index);

            private:
                //gui service event handlers

                void new_conversation_event(const std::string& id);
                void quit_conversation_event(const std::string& id);
                void conversation_synced_event(const fire::message::message& m);
                void contact_removed_or_added_from_conversation_event(const fire::message::message& e);
                void new_contact_event(const std::string& id);
                void contact_connected_event(const user::event::contact_connected&);
                void contact_disconnected_event(const user::event::contact_disconnected&);
                void apps_updated_event(const app::event::apps_updated&);
                void new_intro_event(const user::event::new_introduction&);
                void conversation_alert_event(const conversation::event::conversation_alert&);

            private:
                QMenu *_main_menu;
                QMenu *_contact_menu;
                QAction *_close_action;
                QAction *_about_action;
                QAction *_contact_list_action;
                QMenu *_conversation_menu;
                QAction *_create_conversation_action;
                QAction *_rename_conversation_action;
                QAction *_quit_conversation_action;
                QMenu *_app_menu;
                QAction *_chat_app_action;
                QAction *_app_editor_action;
                QMenu *_debug_menu;
                QAction *_debug_window_action;

                MainTabs* _conversations;
                QWidget* _start_screen;
                bool _start_screen_attached;

                QWidget* _alert_screen;
                list* _alerts;
                int _alert_tab_index;

                QWidget* _root;
                QVBoxLayout* _layout;

            private:
                mail_service* _mail_service;
                fire::message::post_office_ptr _master;
                fire::message::mailbox_ptr _mail;
                fire::security::encrypted_channels_ptr _encrypted_channels;
                user::user_service_ptr _user_service;
                conversation::conversation_service_ptr _conversation_service;
                app::app_service_ptr _app_service;
                main_window_context _context;
                bool _focus;
        };
    }
}

#endif
