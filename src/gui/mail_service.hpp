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
