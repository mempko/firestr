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

#ifndef FIRESTR_APP_SCRIPT_SAMPLE_H
#define FIRESTR_APP_SCRIPT_SAMPLE_H

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#include "gui/app/lua_script_api.hpp"
#include "gui/app/app_service.hpp"

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>

#include "slb/SLB.hpp"

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class script_sample : public message
            {
                Q_OBJECT

                public:
                    script_sample(app_service_ptr, session::session_ptr);
                    script_sample(const std::string& id, app_service_ptr, session::session_ptr);
                    ~script_sample();

                public:
                    const std::string& id();
                    const std::string& type();
                    fire::message::mailbox_ptr mail();

                public slots:
                    void send_script();
                    void save_app();
                    void check_mail();
                    void scroll_to_bottom(int min, int max);

                private:
                    void init();

                private:
                    std::string _id;
                    app_service_ptr _app_service;
                    session::session_ptr _session;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;
                    user::contact_list _contacts;

                    QTextEdit* _script;
                    QPushButton* _run;
                    QPushButton* _save;

                    lua_script_api_ptr _api;
                    app_ptr _app;
            };
            extern const std::string SCRIPT_SAMPLE;

        }
    }
}

#endif

