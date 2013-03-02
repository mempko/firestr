/*
 * Copyright (C) 2013  Maxim Noah Khailo
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

#include "gui/app/chat_sample.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

#include <QTimer>

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
            const std::string CHAT_SAMPLE = "CHAT_SAMPLE";

            namespace
            {
                const size_t TIMER_SLEEP = 100;//in milliseconds
                const size_t PADDING = 20;
                const std::string MESSAGE = "m";
            }

            struct text_message
            {
                std::string from_id;
                std::string text;
            };

            m::message convert(const text_message& t)
            {
                m::message m;
                m.meta.type = MESSAGE;
                m.data = u::to_bytes(t.text);

                return m;
            }

            void convert(const m::message& m, text_message& t)
            {
                REQUIRE_EQUAL(m.meta.type, MESSAGE);
                t.from_id = m.meta.extra["from_id"].as_string();
                t.text = u::to_str(m.data);
            }

            QWidget* make_message_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            chat_sample::chat_sample(s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _session{session}
            {
                REQUIRE(session);
                init();
            }

            chat_sample::chat_sample(const std::string& id, s::session_ptr session) :
                message{},
                _id{id},
                _session{session}
            {
                REQUIRE(session);
                init();
            }

            chat_sample::~chat_sample()
            {
                INVARIANT(_session);
            }

            void chat_sample::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_session);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_session->user_service(), _mail);

                //message list
                _messages = new list;
                _messages->auto_scroll(true);
                layout()->addWidget(_messages, 0, 0, 1, 2);

                //text edit
                _message = new QLineEdit;
                layout()->addWidget(_message, 1, 0);

                //send button
                _send = new QPushButton{"send"};
                layout()->addWidget(_send, 1, 1);

                connect(_message, SIGNAL(returnPressed()), this, SLOT(send_message()));
                connect(_send, SIGNAL(clicked()), this, SLOT(send_message()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //if no contacts added, disable app
                if(_session->contacts().empty())
                {
                    _messages->add(make_message_widget("alert", "no contacts in session"));
                    _message->setEnabled(false);
                    _send->setEnabled(false);
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

            const std::string& chat_sample::id()
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& chat_sample::type()
            {
                ENSURE_FALSE(CHAT_SAMPLE.empty());
                return CHAT_SAMPLE;
            }

            m::mailbox_ptr chat_sample::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void chat_sample::send_message()
            {
                INVARIANT(_message);
                INVARIANT(_send);
                INVARIANT(_session);

                auto text = gui::convert(_message->text());
                _message->clear();

                auto self = _session->user_service()->user().info().name();
                _messages->add(make_message_widget(self, text));

                text_message tm;
                tm.text = text;

                for(auto c : _session->contacts().list())
                {
                    CHECK(c);
                    _sender->send(c->id(), convert(tm)); 
                }
            }

            void chat_sample::check_mail() 
            try
            {
                INVARIANT(_mail);
                INVARIANT(_session);

                m::message m;
                while(_mail->pop_inbox(m))
                {
                    if(m.meta.type == MESSAGE)
                    {
                        text_message t;
                        convert(m, t);

                        auto c = _session->contacts().by_id(t.from_id);
                        if(!c) continue;

                        _messages->add(make_message_widget(c->name(), t.text));
                        _messages->verticalScrollBar()->scroll(0, _messages->verticalScrollBar()->maximum());
                    }
                    else
                    {
                        std::cerr << "chat sample recieved unknown message `" << m.meta.type << "'" << std::endl;
                    }
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "chat_sample: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "chat_sample: unexpected error in check_mail." << std::endl;
            }
        }
    }
}

