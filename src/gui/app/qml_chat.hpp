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

#ifndef FIRESTR_APP_QML_CHAT_H
#define FIRESTR_APP_QML_CHAT_H

#include "gui/app/qml_generic_app.hpp"
#include "gui/mail_service.hpp"
#include "conversation/conversation_service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#include "util/vclock.hpp"

#include <QVariantList>
#include <QString>

#include <string>
#include <memory>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class qml_chat_app : public qml_generic_app
            {
                Q_OBJECT
                Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
                Q_PROPERTY(QString currentMessage READ currentMessage WRITE setCurrentMessage NOTIFY currentMessageChanged)
                Q_PROPERTY(bool canSend READ canSend NOTIFY canSendChanged)

                public:
                    qml_chat_app(
                            conversation::conversation_service_ptr,
                            conversation::conversation_ptr,
                            QObject* parent = nullptr);
                    qml_chat_app(
                            const std::string& id, 
                            conversation::conversation_service_ptr,
                            conversation::conversation_ptr,
                            QObject* parent = nullptr);
                    ~qml_chat_app();

                public:
                    virtual void start() override;
                    virtual void contact_quit(const std::string& id) override;
                    virtual const std::string& id() const override;
                    virtual const std::string& type() const override;
                    virtual fire::message::mailbox_ptr mail() override;
                    
                    // QML properties
                    QVariantList messages() const;
                    QString currentMessage() const;
                    void setCurrentMessage(const QString& msg);
                    bool canSend() const;

                public slots:
                    void sendMessage();
                    void check_mail(fire::message::message);

                signals:
                    void messagesChanged();
                    void currentMessageChanged();
                    void canSendChanged();
                    void messageReceived(const QString& sender, const QString& text, const QString& time);

                private:
                    void init();
                    void send_all(const fire::message::message&);
                    void join();
                    void contact_joined(const std::string& id);
                    void add_message(const QString& sender, const QString& text, const QString& time, bool fromMe = false);

                private:
                    std::string _id;
                    mail_service* _mail_service;
                    util::tracked_sclock _clock;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;
                    
                    // QML data
                    QVariantList _messages;
                    QString _current_message;
            };
            
            extern const std::string QML_CHAT;
        }
    }
}

#endif