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

#include "gui/app/script_sample.hpp"
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
                const std::string MESSAGE = "script";
            }

            struct text_script
            {
                std::string from_id;
                std::string text;
            };

            m::message convert(const text_script& t)
            {
                m::message m;
                m.meta.type = MESSAGE;
                m.data = u::to_bytes(t.text);

                return m;
            }

            void convert(const m::message& m, text_script& t)
            {
                REQUIRE_EQUAL(m.meta.type, MESSAGE);
                t.from_id = m.meta.extra["from_id"].as_string();
                t.text = u::to_str(m.data);
            }

            QWidget* make_script_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            script_sample::script_sample(s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _session{session}
            {
                REQUIRE(session);
                bind();
                init();
            }

            script_sample::script_sample(const std::string& id, s::session_ptr session) :
                message{},
                _id{id},
                _session{session}
            {
                REQUIRE(session);
                bind();
                init();
            }

            script_sample::~script_sample()
            {
                INVARIANT(_session);
            }

            void script_sample::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_session);

                _mail.reset(new m::mailbox{_id});
                _sender.reset(new ms::sender{_session->user_service(), _mail});

                //message list
                _output = new list;
                QObject::connect(_output->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));
                layout()->addWidget(_output, 0, 0, 1, 2);

                //text edit
                _script = new QTextEdit;
                layout()->addWidget(_script, 1, 0, 1, 2);

                //send button
                _run = new QPushButton{"run"};
                layout()->addWidget(_run, 2, 1);

                connect(_run, SIGNAL(clicked()), this, SLOT(send_script()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //if no contacts added, disable app
                if(_session->contacts().empty())
                {
                    _output->add(make_script_widget("alert", "no contacts in session"));
                    _script->setEnabled(false);
                    _run->setEnabled(false);
                    return;
                }

                //setup message timer
                auto *t = new QTimer(this);
                connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
                t->start(TIMER_SLEEP);

                INVARIANT(_session);
                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            void script_sample::bind()
            {  
                using namespace std::placeholders;
                SLB::Class<script_sample, SLB::Instance::NoCopyNoDestroy>{"App", &_m}
                    .set("print", &script_sample::print);

                _state.reset(new SLB::Script{&_m});
                _state->set("app", this);
                INVARIANT(_state);
            }


            void script_sample::print(const std::string& a)
            {
                auto self = _session->user_service()->user().info().name();
                _output->add(make_script_widget(self, a));
            }

            const std::string& script_sample::id()
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& script_sample::type()
            {
                ENSURE_FALSE(SCRIPT_SAMPLE.empty());
                return SCRIPT_SAMPLE;
            }

            m::mailbox_ptr script_sample::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            std::string script_sample::execute(const std::string& s)
            try
            {
                INVARIANT(_state);
                _state->doString(s.c_str());
                return "";
            }
            catch(std::exception& e)
            {
                return e.what();
            }
            catch(...)
            {
                return "unknown";
            }

            void script_sample::run(const std::string name, const std::string& code)
            {
                _output->add(make_script_widget(name, "code: " + code));
                auto error = execute(code);
                if(!error.empty()) _output->add(make_script_widget(name, "error: " + error));
            }

            void script_sample::send_script()
            {
                INVARIANT(_script);
                INVARIANT(_run);
                INVARIANT(_session);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                _script->clear();

                //send the code
                text_script tm;
                tm.text = code;

                for(auto c : _session->contacts().list())
                {
                    CHECK(c);
                    _sender->send(c->id(), convert(tm)); 
                }

                //run the code
                auto self = _session->user_service()->user().info().name();
                run(self, code);
            }

            void script_sample::check_mail() 
            try
            {
                INVARIANT(_mail);
                INVARIANT(_session);

                m::message m;
                while(_mail->pop_inbox(m))
                {
                    if(m.meta.type == MESSAGE)
                    {
                        text_script t;
                        convert(m, t);

                        auto c = _session->contacts().by_id(t.from_id);
                        if(!c) continue;

                        run(c->name(), t.text);
                        _output->verticalScrollBar()->scroll(0, _output->verticalScrollBar()->maximum());
                    }
                    else
                    {
                        std::cerr << "chat sample recieved unknown message `" << m.meta.type << "'" << std::endl;
                    }
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "script_sample: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "script_sample: unexpected error in check_mail." << std::endl;
            }

            void script_sample::scroll_to_bottom(int min, int max)
            {
                Q_UNUSED(min);
                INVARIANT(_output);
                _output->verticalScrollBar()->setValue(max);
            }

        }
    }
}

