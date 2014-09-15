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
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#ifndef FIRESTR_GUI_CONVERSATION_H
#define FIRESTR_GUI_CONVERSATION_H

#include "gui/messagelist.hpp"
#include "gui/contactlist.hpp"
#include "gui/contactselect.hpp"
#include "gui/mail_service.hpp"
#include "gui/app/app_service.hpp"
#include "conversation/conversation.hpp"
#include "conversation/conversation_service.hpp"

#include <QWidget>
#include <QGridLayout>
#include <QComboBox>
#include <QSplitter>

namespace fire
{
    namespace gui
    {
        class conversation_widget : public QWidget
        {
            Q_OBJECT
            public:
                conversation_widget(
                        conversation::conversation_service_ptr, 
                        conversation::conversation_ptr,
                        app::app_service_ptr);
                ~conversation_widget();

            public:
                conversation::conversation_ptr conversation();

                void name(const QString&);
                QString name() const;

            public:
                void add_chat_app();
                void add_app_editor(const std::string& id);
                void add_script_app(const std::string& id);

            public slots:
                void check_mail(fire::message::message); 
                void add(message*);
                void add(QWidget*);
                void add_contact();
                void update_contacts();
                void update_contact_select();

            private:
                void sync_apps();
                void got_req_app_message(const messages::request_app&);

            private:
                mail_service* _mail_service;
                QGridLayout* _layout;
                contact_select_widget* _contact_select;
                QPushButton* _add_contact;
                QSplitter* _splitter;
                QString _name;
                message_list* _messages;
                contact_list* _contact_list;
                conversation::conversation_ptr _conversation;
                conversation::conversation_service_ptr _conversation_service;
                app::app_service_ptr _app_service;
        };

    }
}

#endif
