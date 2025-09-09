/*
 * Copyright (C) 2025  Maxim Noah Khailo
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

#include "gui/app/qml_chat.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QVariantMap>
#include <QDateTime>

#include <sstream>
#include <boost/asio/detail/socket_ops.hpp>

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
            const std::string QML_CHAT = "qml_chat_app";

            namespace
            {
                const std::string MESSAGE = "m";
                const std::string JOINED = "j";
            }

            qml_chat_app::qml_chat_app(
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{u::uuid()},
                _clock{_id},
                _conversation_service{conversation_service},
                _conversation{conversation}
            {
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                init();
            }

            qml_chat_app::qml_chat_app(
                    const std::string& id, 
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{id},
                _clock{_id},
                _conversation_service{conversation_service},
                _conversation{conversation}
            {
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                REQUIRE_FALSE(id.empty());
                init();
            }

            qml_chat_app::~qml_chat_app()
            {
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_mail);
            }

            void qml_chat_app::init()
            {
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation_service->user_service(), _mail);

                // Connect to mail service
                _mail_service = new mail_service{_mail, this};
                connect(_mail_service, &mail_service::got_mail, this, &qml_chat_app::check_mail);

                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            void qml_chat_app::start()
            {
                INVARIANT(_conversation);
                INVARIANT(_mail);

                // Start mail service
                _mail_service->start();
                
                // Send join message
                join();
                
                // Add initial system message
                add_message("System", "Chat started", QDateTime::currentDateTime().toString("hh:mm"));
            }

            void qml_chat_app::contact_quit(const std::string& id)
            {
                INVARIANT(_conversation);
                if(_conversation->contacts().by_id(id))
                {
                    add_message("System", 
                        QString::fromStdString(id) + " left the conversation", 
                        QDateTime::currentDateTime().toString("hh:mm"));
                }
            }

            const std::string& qml_chat_app::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& qml_chat_app::type() const
            {
                ENSURE_FALSE(QML_CHAT.empty());
                return QML_CHAT;
            }

            m::mailbox_ptr qml_chat_app::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void qml_chat_app::send_all(const m::message& m)
            {
                INVARIANT(_conversation);
                INVARIANT(_sender);
                
                for(const auto& c : _conversation->contacts().list())
                {
                    if(c) _sender->send(c->id(), m);
                }
            }

            void qml_chat_app::join()
            {
                INVARIANT(_conversation);
                INVARIANT(_mail);

                m::message m;
                m.meta.type = JOINED;
                m.meta.extra["name"] = _conversation_service->user_service()->user().info().name();
                m.meta.extra["convo_id"] = _conversation->id();
                send_all(m);
            }

            void qml_chat_app::contact_joined(const std::string& id)
            {
                INVARIANT(_conversation);
                INVARIANT(_conversation_service);

                auto contact = _conversation->contacts().by_id(id);
                if(contact)
                {
                    add_message("System", 
                        QString::fromStdString(contact->name()) + " joined", 
                        QDateTime::currentDateTime().toString("hh:mm"));
                }
            }

            void qml_chat_app::check_mail(m::message msg)
            {
                INVARIANT(_mail);
                INVARIANT(_conversation);

                if(msg.meta.type == MESSAGE)
                {
                    auto text = QString::fromStdString(u::to_str(msg.data));
                    auto from_val = msg.meta.extra["from"];
                    auto from = QString::fromStdString(from_val.as_string());
                    
                    // Handle clock if present - merge vector clocks
                    if(msg.meta.extra.has("clock"))
                    {
                        try 
                        {
                            auto clock_dict = msg.meta.extra["clock"].as_dict();
                            auto remote_clock = u::to_tracked_sclock(clock_dict);
                            _clock += remote_clock;
                        }
                        catch(const std::exception& e)
                        {
                            LOG << "Failed to parse vector clock: " << e.what() << std::endl;
                        }
                    }
                    
                    add_message(from, text, QDateTime::currentDateTime().toString("hh:mm"));
                }
                else if(msg.meta.type == JOINED)
                {
                    auto from_val = msg.meta.extra["from"];
                    auto from = from_val.as_string();
                    contact_joined(from);
                }
            }

            void qml_chat_app::sendMessage()
            {
                INVARIANT(_conversation);
                INVARIANT(_conversation_service);
                INVARIANT(_mail);

                if(_current_message.trimmed().isEmpty()) return;

                auto text = _current_message;
                
                // Increment vector clock for this message
                ++_clock;

                // Create message
                m::message m;
                m.meta.type = MESSAGE;
                m.meta.extra["from"] = _conversation_service->user_service()->user().info().name();
                m.meta.extra["clock"] = u::to_dict(_clock);
                m.data = u::to_bytes(text.toStdString());

                // Send to all
                send_all(m);

                // Add to our own view
                add_message("You", text, QDateTime::currentDateTime().toString("hh:mm"), true);

                // Clear input
                _current_message.clear();
                emit currentMessageChanged();
                emit canSendChanged();
            }

            void qml_chat_app::add_message(const QString& sender, const QString& text, const QString& time, bool fromMe)
            {
                QVariantMap msg;
                msg["sender"] = sender;
                msg["text"] = text;
                msg["time"] = time;
                msg["fromMe"] = fromMe;
                
                _messages.prepend(msg);
                emit messagesChanged();
                emit messageReceived(sender, text, time);
            }

            QVariantList qml_chat_app::messages() const
            {
                return _messages;
            }

            QString qml_chat_app::currentMessage() const
            {
                return _current_message;
            }

            void qml_chat_app::setCurrentMessage(const QString& msg)
            {
                if(_current_message != msg)
                {
                    _current_message = msg;
                    emit currentMessageChanged();
                    emit canSendChanged();
                }
            }

            bool qml_chat_app::canSend() const
            {
                return !_current_message.trimmed().isEmpty();
            }
        }
    }
}