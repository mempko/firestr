/*
 * Copyright (C) 2012  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vedit_refsion 3 of the License, or
 * (at your option) any later vedit_refsion.
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
            extern const std::string SIMPLE_MESSAGE;
            class simple_message
            {
                public:
                    simple_message(const std::string& t);
                    simple_message(const fire::message::message&);
                    operator fire::message::message() const;
                public:
                    const std::string& text() const;

                private:
                    std::string _text;
            };

            class lua_script_api;

            struct widget_ref
            {
                std::string id;
                lua_script_api* api;

                bool enabled(); 
                void enable();
                void disable();
            };

            struct button_ref : public widget_ref
            {
                std::string callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  
            };
            typedef std::map<std::string, button_ref> button_ref_map;

            struct edit_ref : public widget_ref
            {
                std::string edited_callback;
                std::string finished_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_edited_callback() const { return edited_callback;}
                void set_edited_callback(const std::string&);  

                const std::string& get_finished_callback() const { return finished_callback;}
                void set_finished_callback(const std::string&);  
            };
            typedef std::map<std::string, edit_ref> edit_ref_map;

            struct text_edit_ref : public widget_ref
            {
                std::string id;
                std::string edited_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_edited_callback() const { return edited_callback;}
                void set_edited_callback(const std::string&);  
            };
            typedef std::map<std::string, text_edit_ref> text_edit_ref_map;

            typedef std::map<std::string, QWidget*> widget_map;
            typedef std::shared_ptr<SLB::Script> script_ptr;

            class lua_script_api : public QObject
            {
                Q_OBJECT
                public:
                    lua_script_api(
                            const user::contact_list& con,
                            messages::sender_ptr sender,
                            session::session_ptr session);

                public:
                    gui::list* output;
                    QWidget* canvas;
                    QGridLayout* layout;
                    SLB::Manager manager;
                    script_ptr state;

                    //message
                    user::contact_list contacts;
                    session::session_ptr session;
                    messages::sender_ptr sender;
                    std::string message_callback;

                    //util
                    void bind();
                    std::string execute(const std::string&);
                    void run(const std::string name, const std::string&);
                    void reset_widgets();
                    void message_recieved(const simple_message&);

                    button_ref_map button_refs;
                    edit_ref_map edit_refs;
                    text_edit_ref_map text_edit_refs;

                    //all widgets referenced are stored here
                    widget_map widgets;

                    //API
                    void print(const std::string& a);
                    button_ref place_button(const std::string& title, int r = 0, int c = 0);
                    edit_ref place_edit(const std::string& text, int r = 0, int c = 0);
                    text_edit_ref place_text_edit(const std::string& text, int r = 0, int c = 0);

                    void set_message_callback(const std::string& a);
                    void send_all(const std::string&);

                    public slots:
                        void button_clicked(QString id);
                        void edit_edited(QString id);
                        void edit_finished(QString id);
                        void text_edit_edited(QString id);
            };

            typedef std::shared_ptr<lua_script_api> lua_script_api_ptr;
        }
    }
}

#endif

