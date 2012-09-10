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
                std::string callback;

                std::string get_text() const; 
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

            struct edit_ref
            {
                std::string id;
                std::string text_edited_callback;
                std::string finished_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_text_edited_callback() const { return text_edited_callback;}
                void set_text_edited_callback(const std::string&);  

                const std::string& get_finished_callback() const { return finished_callback;}
                void set_finished_callback(const std::string&);  

                bool enabled(); 
                void enable();
                void disable();

                lua_script_api* api;
            };
            typedef std::map<std::string, edit_ref> edit_ref_map;
            typedef std::map<std::string, QLineEdit*> edit_widget_map;

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

                    void bind();
                    std::string execute(const std::string&);
                    void run(const std::string name, const std::string&);

                    //button_ref code
                    button_ref_map button_refs;
                    button_widget_map button_widgets;
                    QSignalMapper* button_mapper;

                    //edit_ref code
                    edit_ref_map edit_refs;
                    edit_widget_map edit_widgets;
                    QSignalMapper* edit_text_edited_mapper;
                    QSignalMapper* edit_finished_mapper;

                    //API
                    void print(const std::string& a);
                    button_ref make_button(const std::string& title, int r = 0, int c = 0);
                    edit_ref make_edit(const std::string& title, int r = 0, int c = 0);

                    public slots:
                        void button_clicked(QString id);
                        void edit_text_edited(QString id);
                        void edit_finished(QString id);
            };

            typedef std::shared_ptr<lua_script_api> lua_script_api_ptr;
        }
    }
}

#endif

