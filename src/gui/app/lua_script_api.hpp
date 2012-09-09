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

#ifndef FIRESTR_APP_LUA_SCRIPT_API_H
#define FIRESTR_APP_LUA_SCRIPT_API_H

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>

#include "slb/SLB.hpp"

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class lua_script_api;
            struct button_ref
            {
                std::string id;
                std::string text;
                std::string callback;

                const std::string& get_text() const { return text;}
                void set_text(const std::string&);

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  

                bool enabled(); 
                void enable();
                void disable();

                lua_script_api* api;
            };

            typedef std::map<std::string, button_ref> button_ref_map;
            typedef std::map<std::string, QPushButton*> button_widget_map;
            typedef std::shared_ptr<SLB::Script> script_ptr;

            class lua_script_api : public QObject
            {
                Q_OBJECT
                public:
                    lua_script_api(
                            messages::sender_ptr sender,
                            session::session_ptr session);

                public:
                    gui::list* output;
                    QWidget* canvas;
                    QGridLayout* layout;
                    SLB::Manager manager;
                    script_ptr state;

                    session::session_ptr session;
                    messages::sender_ptr sender;

                    button_ref_map button_refs;
                    button_widget_map button_widgets;
                    QSignalMapper* button_mapper;

                    void bind();
                    std::string execute(const std::string&);
                    void run(const std::string name, const std::string&);

                    //exposed functions
                    void print(const std::string& a);
                    button_ref button(const std::string& title, const std::string& callback, int r = 0, int c = 0);

                    public slots:
                        void button_clicked(QString id);
            };

            typedef std::shared_ptr<lua_script_api> lua_script_api_ptr;
        }
    }
}

#endif

