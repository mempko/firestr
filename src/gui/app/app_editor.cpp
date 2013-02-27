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

#include <QtGui>

#include "gui/app/app_editor.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

#include <QTimer>

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string SCRIPT_SAMPLE = "SCRIPT_SAMPLE";

            namespace
            {
                const size_t TIMER_SLEEP = 100; //in milliseconds
                const size_t PADDING = 20;
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

            app_editor::app_editor(app_service_ptr app_service, s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _app_service{app_service},
                _session{session},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app_service);

                init();

                ENSURE(_api);
                ENSURE(_session);
                ENSURE(_app_service);
            }

            app_editor::app_editor(const std::string& id, app_service_ptr app_service, s::session_ptr session) :
                message{},
                _id{id},
                _app_service{app_service},
                _session{session},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app_service);

                init();

                ENSURE(_api);
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
                INVARIANT(_session);
                INVARIANT(_app_service);

                //if the app exists, load it
                if(_app_service->available_apps().count(_id))
                {
                    _app = _app_service->load_app(_id);
                    CHECK_EQUAL(_app->id(), _id);
                }

                //create gui
                _canvas = new QWidget;
                _canvas_layout = new QGridLayout;
                _canvas->setLayout(_canvas_layout);
                _output = new list;
                layout()->addWidget(_canvas, 0, 0, 1, 2);
                layout()->addWidget(_output, 1, 0, 1, 2);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_session->user_service(), _mail);
                _api = std::make_shared<lua_script_api>(_contacts, _sender, _session, _canvas, _canvas_layout, _output);

                //text edit
                _script = new QTextEdit;
                _script->setWordWrapMode(QTextOption::NoWrap);
                _script->setTabStopWidth(40);
                if(_app) _script->setPlainText(_app->code().c_str());
                layout()->addWidget(_script, 2, 0, 1, 2);

                //send button
                _run = new QPushButton{"run"};
                layout()->addWidget(_run, 3, 0);
                connect(_run, SIGNAL(clicked()), this, SLOT(run_script()));

                _save = new QPushButton{"save"};
                layout()->addWidget(_save, 3, 1);
                connect(_save, SIGNAL(clicked()), this, SLOT(save_app()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup message timer
                auto *t = new QTimer(this);
                connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
                t->start(TIMER_SLEEP);

                //send script
                send_script();

                INVARIANT(_session);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_run);
                INVARIANT(_save);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
            }

            const std::string& app_editor::id()
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& app_editor::type()
            {
                ENSURE_FALSE(SCRIPT_SAMPLE.empty());
                return SCRIPT_SAMPLE;
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
                INVARIANT(_run);

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

            void app_editor::run_script()
            {
                INVARIANT(_script);
                INVARIANT(_session);
                INVARIANT(_api);
                INVARIANT(_run);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                if(code.empty()) return;

                send_script();

                //run the code
                _api->reset_widgets();
                _api->run(code);
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
                            "Name Your App",
                            "App Name",
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

                        _script->setText(t.text.c_str());
                        _api->reset_widgets();
                        _api->run(t.text);
                    }
                    else if(m.meta.type == SCRIPT_MESSAGE)
                    {
                        script_message sm{m, _api.get()};
                        _api->message_recieved(sm);
                    }
                    else
                    {
                        std::cerr << "app_editor recieved unknown message `" << m.meta.type << "'" << std::endl;
                    }
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "app_editor: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "app_editor: unexpected error in check_mail." << std::endl;
            }
        }
    }
}

