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

#ifndef FIRESTR_APP_APP_EDITOR_H
#define FIRESTR_APP_APP_EDITOR_H

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "gui/mail_service.hpp"
#include "conversation/conversation_service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#include "gui/lua/api.hpp"
#include "gui/lua/backend_client.hpp"
#include "gui/qtw/frontend.hpp"
#include "gui/qtw/frontend_client.hpp"
#include "gui/app/app_service.hpp"
#include "util/crstring.hpp"

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QCompleter>
#include <QComboBox>
#include <QSignalMapper>
#include <QSyntaxHighlighter>


#include "slb/SLB.hpp"

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            struct highlight_rule
            {
                QTextCharFormat format;
                QRegExp regex;
            };

            using highlight_rules = std::vector<highlight_rule>;
            class lua_highlighter : public QSyntaxHighlighter
            {
                Q_OBJECT
                public:
                    lua_highlighter(QTextDocument* parent = 0);

                public:
                    void highlightBlock(const QString&);

                private:
                    highlight_rules _rules;
            };

            class app_text_editor : public QTextEdit
            {
                Q_OBJECT
                public:
                    app_text_editor(lua::lua_api*);

                protected:
                    void keyPressEvent(QKeyEvent*);

                signals:
                    void keyPressed(QKeyEvent* e);

                public slots:
                    void insert_completion(const QString&);

                private:
                    QString word_under_cursor() const;
                    QString char_left_of_word() const;
                    QString object_left_of_cursor() const;
                    QStringList auto_complete_list(const std::string& obj);
                    void set_auto_complete_model(const QStringList&);

                private:
                    QCompleter* _c;
                    lua::lua_api* _api;
            };

            struct text_script;
            class app_editor : public message
            {
                Q_OBJECT

                public:
                    app_editor(
                            app_service_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr, 
                            app_ptr a = nullptr);
                    app_editor(
                            const std::string& from_id, 
                            const std::string& id, 
                            app_service_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr, 
                            app_ptr a = nullptr);
                    ~app_editor();

                public:
                    const std::string& id() const;
                    const std::string& type() const;
                    fire::message::mailbox_ptr mail();

                public slots:
                    bool run_script();                                        
                    void send_script(bool send_data = true);
                    void send_script_to(const std::string& id);
                    void ask_for_script();
                    void text_typed(QKeyEvent*);
                    void save_app();
                    void export_app();
                    void init_update();
                    void update();
                    void add_data();
                    void load_data_from_file();
                    void data_key_edited();
                    void key_was_clicked(QString);
                    void data_updated();

                private:
                    void init();
                    void init_handlers();
                    void init_code_tab(QGridLayout*);
                    void init_data_tab(QGridLayout*);
                    bool prepare_script_message(text_script& tm, bool send_data);
                    void init_data();
                    void set_app_name();
                    void update_app_code();
                    void update_error(lua::error_info e);
                    void update_status_to_errors();
                    void update_status_to_no_errors();
                    void update_status_to_typing();
                    void update_status_to_waiting();
                    void update_status_to_running();


                signals:
                    void got_code(const fire::message::message&);
                    void got_init(const fire::message::message&);

                private slots:
                    void emit_got_code(const fire::message::message&);
                    void emit_got_init(const fire::message::message&);

                    void received_code(const fire::message::message&);
                    void received_script_init(const fire::message::message&); 

                private:
                    std::string _from_id;
                    std::string _id;
                    app_service_ptr _app_service;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;
                    user::contact_list _contacts;

                    //clock for keeping track of code
                    util::cr_string _code;

                    //code tab
                    app_text_editor* _script;
                    lua_highlighter* _highlighter;
                    QWidget* _canvas;
                    QLabel* _status;
                    QGridLayout* _canvas_layout;
                    list* _output;

                    //data tab
                    QLineEdit* _data_key;
                    QLineEdit* _data_value;
                    QPushButton* _add_button;
                    util::bytes _data_bytes;
                    list* _data_items;

                    qtw::qt_frontend_client_ptr _front;
                    lua::lua_api_ptr _api;
                    lua::backend_client_ptr _back;

                    app_ptr _app;
                    std::string _prev_code;
                    int _prev_pos = 0;
                    enum run_state { CODE_CHANGED, DONE_TYPING, READY};
                    run_state  _run_state;
                    enum start_state { GET_CODE, DONE_START};
                    start_state _started;
            };
            extern const std::string APP_EDITOR;

            class data_item : public QWidget
            {
                Q_OBJECT
                public:
                    data_item(util::disk_store&, const std::string& key);

                public slots:
                    void remove();
                    void key_clicked();

                signals:
                    void key_was_clicked(QString);
                    void data_updated();

                private:
                    QLabel* _key_label;
                    QLabel* _value_label;
                    QPushButton* _rm;
                    const std::string _key;
                    util::disk_store& _d;
            };


        }
    }
}

#endif

