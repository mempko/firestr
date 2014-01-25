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

#include <QtWidgets>

#include "gui/app/app_editor.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
namespace u = fire::util;
namespace l = fire::gui::lua;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string APP_EDITOR = "APP_EDITOR";
            const std::string LUA_KEYWORDS = "\\b(app|and|break|do|else|elseif|end|false|for|function|if|in|local|nil|not|or|repeat|return|then|true|until|while)\\b";
            const std::string LUA_QUOTE = "\".*[^\\\\]\"";
            const std::string LUA_API_KEYWORDS = "\\b(add|remove|size|button|callback|clear|contact|disable|edit|edited_callback|enable|enabled|finished_callback|from|get|label|last_contact|list|message|name|online|place|place_across|print|send|send_to|set|set_text|text|text_edit|total_contacts|when_clicked|when_edited|when_finished|when_message_received|draw|line|circle|when_mouse_moved|when_mouse_pressed|when_mouse_released|when_mouse_dragged|clear|pen|get_pen|when_local_message_received|is_local|send_local|timer|interval|stop|start|running|when_triggered|save_file|save_bin_file|open_file|open_bin_file|id|str|get_bin|set_bin|sub|append|grow|width|height|grid|alert|good|image|data)\\b";
            const std::string LUA_NUMBERS = "[0-9\\.]+";
            const std::string LUA_OPERATORS = "[=+-\\*\\^:%#~<>\\(\\){}\\[\\];:,]+";

            namespace
            {
                const size_t TIMER_SLEEP = 50; //in milliseconds
                const size_t TIMER_UPDATE = 1000; //in milliseconds
                const size_t PADDING = 20;
                const size_t MIN_EDIT_HEIGHT = 500;
                const std::string SCRIPT_CODE_MESSAGE = "script";
            }

            struct text_script
            {
                std::string from_id;
                std::string text;
            };

            m::message convert(const text_script& t)
            {
                m::message m;
                m.meta.type = SCRIPT_CODE_MESSAGE;
                m.data = u::to_bytes(t.text);

                return m;
            }

            void convert(const m::message& m, text_script& t)
            {
                REQUIRE_EQUAL(m.meta.type, SCRIPT_CODE_MESSAGE);
                t.from_id = m.meta.extra["from_id"].as_string();
                t.text = u::to_str(m.data);
            }

            app_editor::app_editor(
                    app_service_ptr app_service, 
                    s::session_service_ptr session_s, 
                    s::session_ptr session, 
                    app_ptr app) :
                message{},
                _id{u::uuid()},
                _app_service{app_service},
                _session_service{session_s},
                _session{session},
                _contacts{session->contacts()},
                _app{app},
                _prev_pos{0},
                _run_state{READY}
            {
                REQUIRE(app_service);
                REQUIRE(session_s);
                REQUIRE(session);

                init();

                ENSURE(_api);
                ENSURE(_session_service);
                ENSURE(_session);
                ENSURE(_app_service);
            }

            app_editor::app_editor(
                    const std::string& id, 
                    app_service_ptr app_service, 
                    s::session_service_ptr session_s, 
                    s::session_ptr session,
                    app_ptr app) :
                message{},
                _id{id},
                _app_service{app_service},
                _session_service{session_s},
                _session{session},
                _contacts{session->contacts()},
                _app{app}
            {
                REQUIRE(app_service);
                REQUIRE(session_s);
                REQUIRE(session);

                init();

                ENSURE(_api);
                ENSURE(_session_service);
                ENSURE(_session);
                ENSURE(_app_service);
            }

            app_editor::~app_editor()
            {
                INVARIANT(_session);
            }

            void app_editor::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_session_service);
                INVARIANT(_session);
                INVARIANT(_app_service);

                //create gui
                _canvas = new QWidget;
                _canvas_layout = new QGridLayout;
                _canvas->setLayout(_canvas_layout);
                _output = new list;
                layout()->addWidget(_canvas, 0, 0, 1, 2);
                layout()->addWidget(_output, 1, 0, 1, 2);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_session->user_service(), _mail);
                _api = std::make_shared<l::lua_api>(_contacts, _sender, _session, _session_service, _canvas, _canvas_layout, _output);

                //text edit
                _script = new QTextEdit;
                _script->setMinimumHeight(MIN_EDIT_HEIGHT);
                _script->setWordWrapMode(QTextOption::NoWrap);
                _script->setTabStopWidth(40);
                _highlighter = new lua_highlighter(_script->document());

                if(_app) _script->setPlainText(_app->code().c_str());
                layout()->addWidget(_script, 2, 0, 1, 2);

                //add status bar
                _status = new QLabel;
                layout()->addWidget(_status, 3, 0);

                //save button
                _save = new QPushButton{tr("save")};
                layout()->addWidget(_save, 3, 1);
                connect(_save, SIGNAL(clicked()), this, SLOT(save_app()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup message timer
                auto *t = new QTimer(this);
                connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
                t->start(TIMER_SLEEP);

                auto *t2 = new QTimer(this);
                connect(t2, SIGNAL(timeout()), this, SLOT(update()));
                t2->start(TIMER_UPDATE);

                //send script
                run_script();

                INVARIANT(_session);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_save);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
                INVARIANT(_status);
            }

            const std::string& app_editor::id()
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& app_editor::type()
            {
                ENSURE_FALSE(APP_EDITOR.empty());
                return APP_EDITOR;
            }

            m::mailbox_ptr app_editor::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void app_editor::send_script()
            {
                INVARIANT(_script);
                INVARIANT(_session);
                INVARIANT(_api);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                if(code.empty()) return;

                //send the code
                text_script tm;
                tm.text = code;

                for(auto c : _contacts.list())
                {
                    CHECK(c);
                    _sender->send(c->id(), convert(tm)); 
                }
            }

            bool app_editor::run_script()
            {
                INVARIANT(_script);
                INVARIANT(_session);
                INVARIANT(_api);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                if(code.empty()) return true;

                //run the code
                _api->reset_widgets();
                _api->run(code);
                update_error(_api->get_error());
                bool has_no_errors = _api->get_error().line == -1;
                if(has_no_errors) update_status_to_no_errors();
                else update_status_to_errors();
                return has_no_errors;
            }
            
            void app_editor::update_error(l::error_info e)
            {
                INVARIANT(_script);

                QList<QTextEdit::ExtraSelection> extras;
                if(e.line != -1)
                {
                    int line = e.line - 2;
                    if(line < 0) line = 0;
                    QTextEdit::ExtraSelection h;
                    QTextCursor c = _script->textCursor();
                    c.movePosition(QTextCursor::Start);
                    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line); 
                    h.cursor = c;
                    h.format.setProperty(QTextFormat::FullWidthSelection, true);
                    QBrush b{QColor{255, 0, 0, 50}};
                    h.format.setBackground(b);
                    extras << h;
                    update_status_to_errors();
                }
                _script->setExtraSelections( extras );
            }

            void app_editor::save_app() 
            {
                INVARIANT(_session);
                INVARIANT(_app_service);

                if(!_app)
                {
                    bool ok = false;
                    std::string name = "";

                    QString r = QInputDialog::getText(
                            0, 
                            tr("Name Your App"),
                            tr("App Name"),
                            QLineEdit::Normal, name.c_str(), &ok);

                    if(ok && !r.isEmpty()) name = gui::convert(r);
                    else return;

                    _app = std::make_shared<app>();
                    _app->name(name);
                }

                CHECK(_app);

                auto code = gui::convert(_script->toPlainText());
                _app->code(code);
                _app_service->save_app(*_app);
            }

            void app_editor::update_status_to_errors()
            {
                INVARIANT(_status);
                _status->setText(tr("<font color='red'>errors</font>"));
            }

            void app_editor::update_status_to_no_errors()
            {
                INVARIANT(_status);
                _status->setText(tr("<font color='green'>no errors</font>"));
            }

            void app_editor::update_status_to_typing()
            {
                INVARIANT(_status);
                _status->setText(tr("<font color='orange'>typing...</font>"));
            }

            void app_editor::update_status_to_waiting()
            {
                INVARIANT(_status);
                _status->setText(tr("<font color='orange'>waiting...</font>"));
            }

            void app_editor::update_status_to_running()
            {
                INVARIANT(_status);
                _status->setText(tr("<font color='red'>running...</font>"));
            }

            void app_editor::update()
            {
                INVARIANT(_script);
                INVARIANT(_api);
                update_error(_api->get_error());

                auto code = gui::convert(_script->toPlainText());
                int pos = _script->textCursor().position();

                switch(_run_state)
                {
                    case READY:
                        {
                            if(code != _prev_code) 
                            {
                                update_status_to_typing();
                                _run_state = CODE_CHANGED;
                            }
                            break;
                        }
                    case CODE_CHANGED:
                        {
                            if(pos == _prev_pos) 
                            {
                                update_status_to_waiting();
                                _run_state = DONE_TYPING;
                            }
                            break;
                        }
                    case DONE_TYPING:
                        {
                            if(pos == _prev_pos) 
                            {
                                update_status_to_running();

                                //update status bar
                                run_script();
                                send_script();
                            }
                            _run_state = READY;
                            break;
                        }
                }

                _prev_code = code;
                _prev_pos = pos;
            }

            void app_editor::check_mail() 
            try
            {
                INVARIANT(_mail);
                INVARIANT(_session);

                m::message m;
                while(_mail->pop_inbox(m))
                {
                    if(m.meta.type == SCRIPT_CODE_MESSAGE)
                    {
                        text_script t;
                        convert(m, t);

                        auto c = _contacts.by_id(t.from_id);
                        if(!c) continue;

                        auto code = gui::convert(_script->toPlainText());
                        if(t.text == code) continue;

                        //update text
                        auto pos = _script->textCursor().position();
                        _script->setText(t.text.c_str());

                        //put cursor back
                        auto cursor = _script->textCursor();
                        cursor.setPosition(pos);
                        _script->setTextCursor(cursor);

                        //run code
                        _prev_code = t.text;
                        _run_state = READY;
                        run_script();
                    }
                    else if(m.meta.type == l::SCRIPT_MESSAGE)
                    {
                        l::script_message sm{m, _api.get()};
                        _api->message_received(sm);
                    }
                    else
                    {
                        LOG << "app_editor received unknown message `" << m.meta.type << "'" << std::endl;
                    }
                }
            }
            catch(std::exception& e)
            {
                LOG << "app_editor: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "app_editor: unexpected error in check_mail." << std::endl;
            }

            lua_highlighter::lua_highlighter(QTextDocument* parent) :
                QSyntaxHighlighter{parent}
            {
                //keywords
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkBlue);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_KEYWORDS.c_str()};
                    _rules.emplace_back(r);
                }

                //api
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkMagenta);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_API_KEYWORDS.c_str()};
                    _rules.emplace_back(r);
                }

                //numbers
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkRed);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_NUMBERS.c_str()};
                    _rules.emplace_back(r);
                }

                //operators
                {
                    highlight_rule r;
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_OPERATORS.c_str()};
                    _rules.emplace_back(r);
                }

                //quote
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkGreen);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_QUOTE.c_str()};
                    _rules.emplace_back(r);
                }
            }

            void lua_highlighter::highlightBlock(const QString& t)
            {
                for(auto& r : _rules)
                {
                    int i = t.indexOf(r.regex);
                    while (i >= 0)  
                    {
                        auto size = r.regex.matchedLength();
                        setFormat(i, size, r.format);
                        i = t.indexOf(r.regex, i + size);
                    }
                }
            }
        }
    }
}

