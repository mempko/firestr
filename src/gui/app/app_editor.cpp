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
#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/string.hpp"
#include "util/uuid.hpp"

#include <QTimer>

#include <functional>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace l = fire::gui::lua;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string APP_EDITOR = "APP_EDITOR";
            const std::string LUA_KEYWORDS = "\\b(app|and|break|do|else|elseif|end|false|for|function|if|in|local|nil|not|or|repeat|return|then|true|until|while|pairs)\\b";
            const std::string LUA_QUOTE = "\".*[^\\\\]\"";

            const u::string_vect API_KEYWORDS{
                "add",
                "remove",
                "size",
                "button",
                "callback",
                "clear",
                "contact",
                "disable",
                "edit",
                "edited_callback",
                "enable",
                "enabled",
                "finished_callback",
                "from",
                "get",
                "label",
                "last_contact",
                "list",
                "message",
                "name",
                "online",
                "place",
                "place_across",
                "print",
                "send",
                "send_to",
                "set",
                "set_text",
                "text",
                "set_image",
                "text_edit",
                "total_contacts",
                "when_clicked",
                "when_edited",
                "when_finished",
                "when_message_received",
                "draw",
                "line",
                "circle",
                "when_mouse_moved",
                "when_mouse_pressed",
                "when_mouse_released",
                "when_mouse_dragged",
                "clear",
                "pen",
                "get_pen",
                "when_local_message_received",
                "is_local",
                "send_local",
                "timer",
                "interval",
                "stop",
                "start",
                "running",
                "when_triggered",
                "save_file",
                "save_bin_file",
                "open_file",
                "open_bin_file",
                "id",
                "str",
                "get_bin",
                "set_bin",
                "sub",
                "append",
                "grow",
                "width",
                "height",
                "grid",
                "alert",
                "good",
                "image",
                "data",
                "store",
                "i_started",
                "who_started",
                "self"
            };

            const std::string LUA_NUMBERS = "[0-9\\.]+";
            const std::string LUA_OPERATORS = "[=+-\\*\\^:%#~<>\\(\\){}\\[\\];:,]+";

            namespace
            {
                const size_t TIMER_UPDATE = 1000; //in milliseconds
                const size_t PADDING = 20;
                const size_t MIN_EDIT_HEIGHT = 500;
                const std::string SCRIPT_CODE_MESSAGE = "script";
            }

            struct text_script
            {
                std::string from_id;
                std::string code;
                u::bytes data;
            };

            m::message convert(const text_script& t)
            {
                m::message m;
                m.meta.type = SCRIPT_CODE_MESSAGE;
                m.meta.extra["code"] = t.code;
                m.data = t.data;

                return m;
            }

            void convert(const m::message& m, text_script& t)
            {
                REQUIRE_EQUAL(m.meta.type, SCRIPT_CODE_MESSAGE);
                t.from_id = m.meta.extra["from_id"].as_string();
                t.code = m.meta.extra["code"].as_string();
                t.data = m.data;
            }


            app_editor::app_editor(
                    app_service_ptr app_service, 
                    s::conversation_service_ptr conversation_s, 
                    s::conversation_ptr conversation, 
                    app_ptr app) :
                message{},
                _from_id{conversation->user_service()->user().info().id()},
                _id{u::uuid()},
                _app_service{app_service},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _contacts{conversation->contacts()},
                _app{app},
                _prev_pos{0},
                _run_state{READY}
            {
                REQUIRE(app_service);
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);

                init();

                INVARIANT(_app);
                ENSURE(_api);
                ENSURE(_conversation_service);
                ENSURE(_conversation);
                ENSURE(_app_service);
            }

            app_editor::app_editor(
                    const std::string& from_id, 
                    const std::string& id, 
                    app_service_ptr app_service, 
                    s::conversation_service_ptr conversation_s, 
                    s::conversation_ptr conversation,
                    app_ptr app) :
                message{},
                _from_id{from_id},
                _id{id},
                _app_service{app_service},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _contacts{conversation->contacts()},
                _app{app}
            {
                REQUIRE(app_service);
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);

                init();

                INVARIANT(_app);
                ENSURE(_api);
                ENSURE(_conversation_service);
                ENSURE(_conversation);
                ENSURE(_app_service);
            }

            app_editor::~app_editor()
            {
                INVARIANT(_conversation);
                INVARIANT(_mail_service);
                _mail_service->done();
            }

            void app_editor::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_app);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation->user_service(), _mail);

                //create gui
                auto tabs = new QTabWidget{this};
                layout()->addWidget(tabs);

                //code tab
                auto code_tab = new QWidget{this};
                auto code_layout = new QGridLayout{code_tab};
                tabs->addTab(code_tab, tr("code"));

                init_code_tab(code_layout);

                //data tab
                auto data_tab = new QWidget{this};
                auto data_layout = new QGridLayout{data_tab};
                tabs->addTab(data_tab, tr("data"));

                init_data_tab(data_layout);

                //run app
                run_script();

                INVARIANT(_app);
                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_save);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
                INVARIANT(_status);
            }

            void app_editor::init_code_tab(QGridLayout* l)
            {
                REQUIRE(l);
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);

                _canvas = new QWidget;
                _canvas_layout = new QGridLayout;
                _canvas->setLayout(_canvas_layout);
                _output = new list;
                l->addWidget(_canvas, 0, 0, 1, 2);
                l->addWidget(_output, 1, 0, 1, 2);

                _api = std::make_shared<l::lua_api>(_app, _contacts, _sender, _conversation, _conversation_service, _canvas, _canvas_layout, _output);

                //text edit
                _script = new QTextEdit;
                _script->setMinimumHeight(MIN_EDIT_HEIGHT);
                _script->setWordWrapMode(QTextOption::NoWrap);
                _script->setTabStopWidth(40);
                _highlighter = new lua_highlighter(_script->document());

                _script->setPlainText(_app->code().c_str());
                l->addWidget(_script, 2, 0, 1, 2);

                //add status bar
                _status = new QLabel;
                l->addWidget(_status, 3, 0);

                //save button
                _save = new QPushButton{tr("save")};
                l->addWidget(_save, 3, 1);
                connect(_save, SIGNAL(clicked()), this, SLOT(save_app()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup mail service
                _mail_service = new mail_service{_mail, this};
                _mail_service->start();

                //setup update timer
                auto *t2 = new QTimer(this);
                connect(t2, SIGNAL(timeout()), this, SLOT(update()));
                t2->start(TIMER_UPDATE);

                INVARIANT(_app);
                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_save);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
                INVARIANT(_status);
                INVARIANT(_mail_service);
            }

            void app_editor::init_data()
            {
                INVARIANT(_data_items);
                INVARIANT(_app);

                _data_items->clear();

                for(const auto& p : _app->data())
                {
                    auto di = new data_item(_app->data(), p.first);
                    connect(di, SIGNAL(key_was_clicked(QString)), this, SLOT(key_was_clicked(QString)));
                    _data_items->add(di);
                }

                ENSURE_EQUAL(_data_items->size(), _app->data().size());
            }

            void app_editor::init_data_tab(QGridLayout* l)
            {
                REQUIRE(l);
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_app);

                auto key_label = new QLabel{tr("key:")};
                auto value_label = new QLabel{tr("value:")};
                _data_key = new QLineEdit;
                connect(_data_key, SIGNAL(textChanged(QString)), this, SLOT(data_key_edited()));

                auto value_widget = new QWidget;
                auto value_layout = new QHBoxLayout{value_widget};
                _data_value = new QLineEdit;
                connect(_data_value, SIGNAL(returnPressed()), this, SLOT(add_data()));

                auto load_button = new QPushButton{tr("...")};
                load_button->setMaximumSize(20,20);
                connect(load_button, SIGNAL(clicked()), this, SLOT(load_data_from_file()));

                value_layout->addWidget(_data_value);
                value_layout->addWidget(load_button);

                _add_button = new QPushButton{tr("+")};
                _add_button->setMaximumSize(20,20);
                connect(_add_button, SIGNAL(clicked()), this, SLOT(add_data()));

                l->addWidget(key_label, 0, 0);
                l->addWidget(_data_key, 0, 1);
                l->addWidget(value_label, 0, 2);
                l->addWidget(value_widget, 0, 3);
                l->addWidget(_add_button, 0, 4);

                _data_items = new list;
                l->addWidget(_data_items, 1, 0, 5, 0);

                init_data();
                data_key_edited();

                ENSURE(_data_key);
                ENSURE(_data_value);
                ENSURE(_add_button);
                ENSURE(_data_items);
            }

            void app_editor::data_key_edited()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                bool enabled = !_data_key->text().isEmpty();
                _data_value->setEnabled(enabled);
                _add_button->setEnabled(enabled);
            }

            void app_editor::key_was_clicked(QString key)
            {
                INVARIANT(_data_key);
                _data_key->setText(key);
            }

            void app_editor::add_data()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                INVARIANT(_app);
                auto k = gui::convert(_data_key->text());
                u::trim(k);
                auto vt = gui::convert(_data_value->text());
                u::trim(vt);
                u::value v = vt;

                //must have a key
                if(k.empty()) return;

                //silly check to see loaded a file or set text value
                if(vt == "<file>") _app->data().set(k, _data_bytes);
                else 
                {
                    //try to convert to number, if not a number then keep string.
                    try { v = boost::lexical_cast<double>(vt); }catch(...){}

                    _app->data().set(k, v);
                }

                //refresh ui
                init_data();

                //clear input
                _data_key->clear();
                _data_value->clear();
                _data_bytes.clear();
                _data_key->setFocus(Qt::OtherFocusReason);

                _run_state = CODE_CHANGED;
            }

            void app_editor::load_data_from_file()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                auto sf = get_file_name(this);
                if(sf.empty()) return;

                if(!load_from_file(sf, _data_bytes)) return;

                _data_value->setText("<file>");
                if(_data_key->text().isEmpty())
                {
                    bf::path p = sf;
                    _data_key->setText(p.filename().string().c_str());
                }
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
                INVARIANT(_conversation);
                INVARIANT(_api);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                if(code.empty()) return;

                //set the code
                text_script tm;
                tm.code = code;

                //export the data
                u::dict data;
                _app->data().export_to(data);
                tm.data = u::encode(data);

                //send it all
                for(auto c : _contacts.list())
                {
                    CHECK(c);
                    _sender->send(c->id(), convert(tm)); 
                }
            }

            bool app_editor::run_script()
            {
                INVARIANT(_script);
                INVARIANT(_conversation);
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

                //update ui
                init_data();

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
                INVARIANT(_conversation);
                INVARIANT(_app_service);

                if(_app->name().empty())
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
                            if(pos == _prev_pos && code == _prev_code) 
                            {
                                update_status_to_waiting();
                                _run_state = DONE_TYPING;
                            }
                            break;
                        }
                    case DONE_TYPING:
                        {
                            if(pos == _prev_pos && code == _prev_code) 
                            {
                                update_status_to_running();

                                //update status bar
                                run_script();
                                send_script();
                                _run_state = READY;
                            } 
                            else
                            {
                                if(code != _prev_code) 
                                {
                                    update_status_to_typing();
                                    _run_state = CODE_CHANGED;
                                }
                                else
                                {
                                    update_status_to_waiting();
                                    _run_state = DONE_TYPING;
                                }
                            }

                            break;
                        }
                }

                _prev_code = code;
                _prev_pos = pos;
            }

            void app_editor::check_mail(m::message m) 
            try
            {
                INVARIANT(_mail);
                INVARIANT(_conversation);

                if(m::is_remote(m)) m::expect_symmetric(m);
                else m::expect_plaintext(m);

                if(m.meta.type == SCRIPT_CODE_MESSAGE)
                {
                    text_script t;
                    convert(m, t);

                    auto c = _contacts.by_id(t.from_id);
                    if(!c) return;

                    auto code = gui::convert(_script->toPlainText());
                    if(t.code == code && t.data.empty()) return;

                    //update text
                    auto pos = _script->textCursor().position();
                    _script->setText(t.code.c_str());

                    //update data
                    u::dict data = u::decode<u::dict>(t.data);
                    _app->data().import_from(data);

                    //put cursor back
                    auto cursor = _script->textCursor();
                    cursor.setPosition(pos);
                    _script->setTextCursor(cursor);

                    //update data ui
                    init_data();

                    //run code
                    _prev_code = t.code;
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
                //create api keyword regex
                std::string api_keywords = "\\b(";
                for(int i = 0; i < API_KEYWORDS.size();i++)
                {
                    if(i != 0) api_keywords.append("|");
                    api_keywords.append(API_KEYWORDS[i]);
                }
                api_keywords.append(")\\b");

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
                    r.regex = QRegExp{api_keywords.c_str()};
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

            data_item::data_item(util::disk_store& d, const std::string& key) :
                _key{key}, _d(d)
            {
                auto l = new QHBoxLayout{this};
                setLayout(l);

                std::stringstream ks;
                ks << "<a href='" << key <<"'>" << key << "</a>";
                _key_label = new QLabel{ks.str().c_str()};

                auto v = d.get(key);

                std::stringstream vs;
                if(v.is_int()) vs << v.as_int();
                else if(v.is_size()) vs << v.as_size();
                else if(v.is_double()) vs << v.as_double();
                else if(v.is_dict()) vs << "<complext data>";
                else if(v.is_array()) vs << "<array>";
                else if(v.is_bytes()) 
                {
                    auto b = v.as_bytes();
                    if(b.size() > 100) vs << "<file>";
                    else vs << u::to_str(b);
                }

                _value_label = new QLabel{vs.str().c_str()};

                _rm = new QPushButton{tr("x")};
                _rm->setMaximumSize(20,20);
                connect(_rm, SIGNAL(clicked()), this, SLOT(remove()));
                connect(_key_label, SIGNAL(linkActivated(QString)), this, SLOT(key_clicked()));

                l->addWidget(_key_label);
                l->addWidget(_value_label);
                l->addWidget(_rm);

                INVARIANT(_key_label);
                INVARIANT(_value_label);
                INVARIANT(_rm);
            }

            void data_item::remove()
            {
                INVARIANT(_key_label);
                INVARIANT(_value_label);
                INVARIANT(_rm);
                _d.remove(_key);
                setEnabled(false);
            }
            void data_item::key_clicked()
            {
                QString k{_key.c_str()};
                emit key_was_clicked(k);
            }
        }
    }
}

