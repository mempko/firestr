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
 */

#ifndef FIRESTR_GUI_MAIL_SERVICE_H
#define FIRESTR_GUI_MAIL_SERVICE_H

#include <QThread>
#include "message/mailbox.hpp"

namespace fire 
{
    namespace gui 
    {
        class mail_service : public QThread
        {
            Q_OBJECT
            public:
                /**
                 * The parent QObject must have a slot of the type
                 * void check_mail(fire::message::message)
                 */
                mail_service(fire::message::mailbox_ptr, QObject* parent);
                ~mail_service();
            public:
                void done();

            private:
                void run();

            signals:
                    void got_mail(fire::message::message);

            private:
                bool _done;
                fire::message::mailbox_ptr _mail;
        };

    }
}

#endif
