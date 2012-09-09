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

            script_sample::script_sample(s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _session{session},
                _api{new api_impl}
                
            {
                REQUIRE(session);

                _api->bind();
                init();

                ENSURE(_api);
            }

            script_sample::script_sample(const std::string& id, s::session_ptr session) :
                message{},
                _id{id},
                _session{session},
                _api{new api_impl}
            {
                REQUIRE(session);

                _api->bind();
                init();

                ENSURE(_api);
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

                //setup api widgets
                _api->sender = _sender;
                _api->session = _session;

                //canvas
                _api->canvas = new QWidget;
                _api->layout = new QGridLayout;
                _api->canvas->setLayout(_api->layout);

                _api->button_mapper = new QSignalMapper(_api->canvas);

                layout()->addWidget(_api->canvas, 0, 0, 1, 2);

                //message list
                _api->output = new list;
                QObject::connect(_api->output->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));
                layout()->addWidget(_api->output, 1, 0, 1, 2);

                //text edit
                _script = new QTextEdit;
                layout()->addWidget(_script, 2, 0, 1, 2);

                //send button
                _run = new QPushButton{"run"};
                layout()->addWidget(_run, 3, 1);

                connect(_run, SIGNAL(clicked()), this, SLOT(send_script()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup message timer
                auto *t = new QTimer(this);
                connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
                t->start(TIMER_SLEEP);

                INVARIANT(_session);
                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            void api_impl::bind()
            {  
                REQUIRE_FALSE(state);

                using namespace std::placeholders;
                SLB::Class<api_impl, SLB::Instance::NoCopyNoDestroy>{"Api", &manager}
                    .set("print", &api_impl::print)
                    .set("button", &api_impl::button);

                SLB::Class<button_ref>{"button", &manager}
                    .set("get_text", &button_ref::get_text)
                    .set("set_text", &button_ref::set_text)
                    .set("get_callback", &button_ref::get_callback)
                    .set("set_callback", &button_ref::set_callback)
                    .set("enabled", &button_ref::enabled)
                    .set("enable", &button_ref::enable)
                    .set("disable", &button_ref::disable);

                state.reset(new SLB::Script{&manager});
                state->set("str", this);

                ENSURE(state);
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

            void script_sample::send_script()
            {
                INVARIANT(_script);
                INVARIANT(_session);
                INVARIANT(_api);
                INVARIANT(_run);

                //get the code
                auto code = gui::convert(_script->toPlainText());
                if(code.empty()) return;

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
                _api->run(self, code);
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

                        _api->run(c->name(), t.text);
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
                INVARIANT(_api->output);
                _api->output->verticalScrollBar()->setValue(max);
            }

            QWidget* make_script_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            std::string api_impl::execute(const std::string& s)
            try
            {
                REQUIRE_FALSE(s.empty());
                INVARIANT(state);

                state->doString(s.c_str());
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

            void api_impl::run(const std::string name, const std::string& code)
            {
                INVARIANT(output);
                REQUIRE_FALSE(code.empty());

                output->add(make_script_widget(name, "code: " + code));
                auto error = execute(code);
                if(!error.empty()) output->add(make_script_widget(name, "error: " + error));
            }


            //API implementation 
            void api_impl::print(const std::string& a)
            {
                INVARIANT(session);
                INVARIANT(output);
                INVARIANT(session->user_service());

                auto self = session->user_service()->user().info().name();
                output->add(make_script_widget(self, a));
            }

            button_ref api_impl::button(const std::string& text, const std::string& callback, int r, int c)
            {
                INVARIANT(layout);
                INVARIANT(button_mapper);

                //create button reference
                button_ref ref{u::uuid(), text, callback, this};

                //create button widget
                auto b = new QPushButton(text.c_str());
                layout->addWidget(b, r, c);

                //map button to C++ callback
                button_mapper->setMapping(b, QString(ref.id.c_str()));
                connect(b, SIGNAL(clicked()), button_mapper, SLOT(map()));
                connect(button_mapper, SIGNAL(mapped(QString)), this, SLOT(button_clicked(QString)));

                //add ref and widget to maps
                button_refs[ref.id] = ref;
                button_widgets[ref.id] = b;

                ENSURE_EQUAL(ref.text, text);
                ENSURE_EQUAL(ref.callback, callback);
                ENSURE(ref.api);
                return ref;
            }

            void api_impl::button_clicked(QString id)
            {
                INVARIANT(state);

                auto rp = button_refs.find(gui::convert(id));
                if(rp == button_refs.end()) return;

                const auto& callback = rp->second.callback;
                if(callback.empty()) return;

                state->call(rp->second.callback);
            }

            QPushButton* get_widget(const button_ref& r, api_impl& api)
            {
                auto wp = api.button_widgets.find(r.id);
                return wp != api.button_widgets.end() ? wp->second : nullptr;
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget(*this, *api);
                CHECK(button);

                rp->second.text = t;
                text = t;
                button->setText(t.c_str());
            }

            void button_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                rp->second.callback = c;
                callback = c;
            }  

            bool button_ref::enabled()
            {
                INVARIANT(api);

                auto button = get_widget(*this, *api);
                if(!button) return false;

                return button->isEnabled();
            }

            void button_ref::enable()
            {
                INVARIANT(api);

                auto button = get_widget(*this, *api);
                if(!button) return;

                button->setEnabled(true);
            }

            void button_ref::disable()
            {
                INVARIANT(api);

                auto button = get_widget(*this, *api);
                if(!button) return;

                button->setEnabled(false);
            }
        }
    }
}

