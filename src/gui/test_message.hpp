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

#ifndef FIRESTR_GUI_TEST_MESSAGE_H
#define FIRESTR_GUI_TEST_MESSAGE_H

#include "messages/test_message.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

namespace fire
{
    namespace gui
    {
        class test_message : public message
        {
            Q_OBJECT
            public:
                test_message(session::session_ptr);
                test_message(const messages::test_message&, session::session_ptr);

            public slots:
                void send_message();
                void send_reply();

            private:
                void init_send();
                void init_reply();

            private:
                messages::test_message _m;
                session::session_ptr _session;
                QLineEdit* _etext;
                QPushButton* _send;
                QPushButton* _reply;
        };
    }

}

#endif

