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

#ifndef FIRESTR_MAINWIN_H
#define FIRESTR_MAINWIN_H

#include <QMainWindow>
#include "gui/conversation.hpp"
#include "gui/app/app_service.hpp"
#include "gui/mail_service.hpp"
#include "message/postoffice.hpp"
#include "user/userservice.hpp"
#include "conversation/conversation.hpp"
#include "conversation/conversation_service.hpp"
#include "security/security_library.hpp"
#include "util/dbc.hpp"

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
                void create_invite();
                void add_contact();
                void show_contact_list();
                void make_chat_app();
                void make_app_editor();
                void install_app();
                void closeEvent(QCloseEvent*);
                void dragEnterEvent(QDragEnterEvent*);
                void dropEvent(QDropEvent*);
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
                void install_app(const std::string& file);

            private:
                //gui message handlers
                void init_handlers();

                void received_new_conversation(const fire::message::message&);
                void received_quit_conversation(const fire::message::message&);
                void received_conversation_synced(const fire::message::message&);
                void received_contact_removed_or_added_from_conversation(const fire::message::message&);
                void received_conversation_alert(const fire::message::message&);
                void received_contact_connected(const fire::message::message&);
                void received_contact_disconnected(const fire::message::message&);
                void received_new_introduction(const fire::message::message&);
                void received_apps_updated(const fire::message::message&);

            private:
                //gui service event handlers

                void new_conversation_event(const std::string& id);
                void quit_conversation_event(const std::string& id);
                void contact_removed_or_added_from_conversation_event(const fire::message::message& e);
                void new_contact_event(const std::string& id);
                void contact_connected_event(const user::event::contact_connected&);
                void contact_disconnected_event(const user::event::contact_disconnected&);
                void apps_updated_event(const app::event::apps_updated&);
                void new_intro_event(const user::event::new_introduction&);
                void conversation_alert_event(const conversation::event::conversation_alert&);

            private:
                QMenu *_main_menu;
                QAction *_close_action;
                QAction *_about_action;

                QMenu *_contact_menu;
                QAction *_create_invite_action;
                QAction *_add_contact_action;
                QAction *_contact_list_action;

                QMenu *_conversation_menu;
                QAction *_create_conversation_action;
                QAction *_rename_conversation_action;
                QAction *_quit_conversation_action;

                QMenu *_app_menu;
                QAction *_chat_app_action;
                QAction *_app_editor_action;
                QAction *_install_app_action;

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
                service::service_map _service_map;
        };
    }
}

#endif
