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

#ifndef FIRESTR_APP_CHAT_H
#define FIRESTR_APP_CHAT_H

#include "gui/list.hpp"
#include "gui/mail_service.hpp"
#include "gui/app/generic_app.hpp"
#include "conversation/conversation_service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class chat_app : public generic_app
            {
                Q_OBJECT

                public:
                    chat_app(
                            conversation::conversation_service_ptr,
                            conversation::conversation_ptr);
                    chat_app(
                            const std::string& id, 
                            conversation::conversation_service_ptr,
                            conversation::conversation_ptr);
                    ~chat_app();

                public:
                    const std::string& id() const;
                    const std::string& type() const;
                    fire::message::mailbox_ptr mail();

                public slots:
                    void send_message();
                    void check_mail(fire::message::message);

                private:
                    void init();

                private:
                    std::string _id;
                    mail_service* _mail_service;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;

                    QWidget* _main;
                    QGridLayout* _main_layout;
                    gui::list* _messages;
                    QLineEdit* _message;
                    QPushButton* _send;
            };
            extern const std::string CHAT;

        }
    }
}

#endif

