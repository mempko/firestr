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

#include <QObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

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
                    script_sample(session::session_ptr);
                    script_sample(const std::string& id, session::session_ptr);
                    ~script_sample();

                public:
                    const std::string& id();
                    const std::string& type();
                    fire::message::mailbox_ptr mail();

                public slots:
                    void send_script();
                    void check_mail();
                    void scroll_to_bottom(int min, int max);

                private:
                    void init();

                private:
                    std::string _id;
                    session::session_ptr _session;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;

                    gui::list* _output;
                    QLineEdit* _script;
                    QPushButton* _run;
            };
            extern const std::string SCRIPT_SAMPLE;

        }
    }
}

#endif

