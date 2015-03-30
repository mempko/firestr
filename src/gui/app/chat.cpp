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

#include <QtWidgets>

#include "gui/app/chat.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/time.hpp"
#include "util/uuid.hpp"
#include "util/serialize.hpp"
#include "util/string.hpp"

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string CHAT = "CHAT_APP";

            namespace
            {
                const size_t PADDING = 200;
                const std::string MESSAGE = "m";
                const std::string JOINED = "j";
            }

            f_message(text_message)
            {
                std::string text;
                u::tracked_sclock clock{""};
                bool has_clock = false;

                f_message_init(text_message, MESSAGE);
                f_serialize_in
                {
                    f_sk("t", text);
                    if(f_has("c")) 
                    {
                        has_clock = true;

                        u::dict cd;
                        f_sk("c", cd);
                        clock = u::to_tracked_sclock(cd);
                    }
                }

                f_serialize_out
                {
                    f_sk("t", text);
                    f_sk("c", u::to_dict(clock));
                }
            };

            f_message(joined_message)
            {
                f_message_init(joined_message, JOINED);
                f_serialize_empty;
            };

            QString make_message_str(const std::string& color, const std::string& name, const std::string& text)
            {
                std::stringstream s;
                s << "<font color='gray'>" << u::hour_min_sec() << "</font> <b><font color='" << color << "'>" << name << "</font></b>: " << text << "<br/>";
                return s.str().c_str();
            }

            chat_app::chat_app(
                    s::conversation_service_ptr conversation_s,
                    s::conversation_ptr conversation) :
                generic_app{},
                _id{u::uuid()},
                _clock{conversation->user_service()->user().info().id()},
                _conversation_service{conversation_s},
                _conversation{conversation}
            {
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                init();
            }

            chat_app::chat_app(
                    const std::string& id, 
                    s::conversation_service_ptr conversation_s,
                    s::conversation_ptr conversation) :
                generic_app{},
                _id{id},
                _clock{conversation->user_service()->user().info().id()},
                _conversation_service{conversation_s},
                _conversation{conversation}
            {
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                init();
            }

            chat_app::~chat_app()
            {
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_mail_service);
                _mail_service->done();
            }

            void chat_app::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation);

                set_title("Chat");

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation->user_service(), _mail);

                _main = new QWidget;
                _main_layout = new QGridLayout{_main};
                layout()->addWidget(_main, 1,0,2,3);

                set_main(_main);

                //message list
                _messages = new QTextEdit;
                _messages->setReadOnly(true);
                _messages->setWordWrapMode(QTextOption::WordWrap);
                _messages->setUndoRedoEnabled(false);
                _main_layout->addWidget(_messages, 0, 0, 1, 2);

                //text edit
                _message = new QLineEdit;
                _main_layout->addWidget(_message, 1, 0);

                //send button
                _send = new QPushButton;
                make_reply(*_send);
                _send->setToolTip(tr("Send"));
                _main_layout->addWidget(_send, 1, 1);

                connect(_message, SIGNAL(returnPressed()), this, SLOT(send_message()));
                connect(_send, SIGNAL(clicked()), this, SLOT(send_message()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup mail service
                _mail_service = new mail_service{_mail, this};
                _mail_service->start();

                //join the chat
                join();

                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            void chat_app::send_all(const m::message& m)
            {
                for(auto c : _conversation->contacts().list())
                {
                    CHECK(c);
                    _sender->send(c->id(), m); 
                }
            }

            void chat_app::join()
            {
                INVARIANT(_sender);
                INVARIANT(_conversation);
                INVARIANT(_conversation->user_service());

                joined_message jm;
                send_all(jm.to_message());
            }

            const std::string& chat_app::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& chat_app::type() const
            {
                ENSURE_FALSE(CHAT.empty());
                return CHAT;
            }

            m::mailbox_ptr chat_app::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void chat_app::add_text(const QString& q)
            {
                INVARIANT(_messages);
                _text.append(q);
                _messages->setHtml(_text);
                _messages->verticalScrollBar()->setValue(_messages->verticalScrollBar()->maximum());
            }


            void chat_app::send_message()
            {
                INVARIANT(_message);
                INVARIANT(_send);
                INVARIANT(_conversation);

                auto text = gui::convert(_message->text());

                ///don't send empty messages
                u::trim(text);
                if(text.empty()) return;

                //update gui
                _message->clear();
                auto self = _conversation->user_service()->user().info().name();
                add_text(make_message_str("blue", self, text));

                //send the message 
                _clock++;

                text_message tm;
                tm.text = text;
                tm.clock = _clock;

                send_all(tm.to_message());
                if(_conversation->contacts().list().empty()) add_text(make_message_str("red", "notice", "nobody here..."));
            }

            void chat_app::check_mail(m::message m) 
            try
            {
                INVARIANT(_conversation);
                INVARIANT(_conversation_service);

                if(m::is_remote(m)) m::expect_symmetric(m);
                else m::expect_plaintext(m);

                if(m.meta.type == MESSAGE)
                {
                    text_message t;
                    t.from_message(m);

                    auto c = _conversation->contacts().by_id(t.from_id);
                    if(!c) return;

                    //older clients do not have clocks
                    if(t.has_clock)
                    {
                        //discard resent messages
                        if(t.clock <= _clock) return;

                        //merge clocks
                        _clock += t.clock;
                    }

                    add_text(make_message_str("black", c->name(), t.text));

                    _conversation_service->fire_conversation_alert(_conversation->id(), visible());
                    alerted();
                }
                else if(m.meta.type == JOINED)
                {
                    joined_message t;
                    t.from_message(m);
                    contact_joined(t.from_id);
                }
                else
                {
                    LOG << "chat sample received unknown message `" << m.meta.type << "'" << std::endl;
                }
            }
            catch(std::exception& e)
            {
                LOG << "chat_app: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "chat_app: unexpected error in check_mail." << std::endl;
            }

            void chat_app::contact_joined(const std::string& id)
            {
                REQUIRE_FALSE(id.empty());
                INVARIANT(_messages);
                INVARIANT(_conversation);

                LOG << id << "joined chat" << std::endl;
                auto c = _conversation->contacts().by_id(id);
                if(!c) return;

                std::stringstream s;
                s << c->name() << " joined";
                add_text(make_message_str("red", "notice", s.str()));
            }

            void chat_app::contact_quit(const std::string& id)
            {
                REQUIRE_FALSE(id.empty());
                INVARIANT(_messages);
                INVARIANT(_conversation);

                auto c = _conversation->user_service()->by_id(id);
                if(!c) return;

                std::stringstream s;
                s << c->name() << " quit";
                add_text(make_message_str("red", "notice", s.str()));
            }
        }
    }
}

