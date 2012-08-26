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

#ifndef FIRESTR_TESTWIN_H
#define FIRESTR_TESTWIN_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>

#include "gui/messagelist.hpp"
#include "message/postoffice.hpp"

namespace fire
{
    namespace gui
    {
        class test_window : public QMainWindow
        {
            Q_OBJECT
            public:
                test_window(
                        const std::string& host, 
                        const std::string& port,
                        const std::string& fire_port);

            private slots:
                void about();
                void send_text();

            private:
                void setup_post(
                        const std::string& host, 
                        const std::string& port);

                void create_actions();
                void create_main();
                void create_menus();

            private:
                QMenu *_main_menu;
                QAction *_close_action;
                QAction *_about_action;

                QWidget* _root;
                QWidget* _ctr_root;
                QGridLayout* _ctr_layout;
                QGridLayout* _layout;

                QLineEdit* _to;
                QLineEdit* _message_edit;
                QPushButton* _send_text;

                message_list* _messages;
                fire::message::post_office_ptr _master;
                std::string _fire_port;
        };
    }
}

#endif
