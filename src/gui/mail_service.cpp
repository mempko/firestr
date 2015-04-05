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

#include "gui/mail_service.hpp"

#include "util/dbc.hpp"
#include "util/log.hpp"

namespace m = fire::message;

namespace fire 
{
    namespace gui 
    {
        mail_service::mail_service(message::mailbox_ptr m, QObject* parent) : 
            QThread{parent},
            _done{false},
            _mail{m}
        {
            REQUIRE(parent);
            REQUIRE(m);
            ENSURE(_mail);
            qRegisterMetaType<m::message>("fire::message::message");
            connect(this, SIGNAL(got_mail(fire::message::message)), parent, SLOT(check_mail(fire::message::message)));
        }

        mail_service::~mail_service()
        {
            done();
        }

        void mail_service::done()
        {
            INVARIANT(_mail);

            if(_done) return;
            _done = true;
            _mail->done();
            wait();
        }

        void mail_service::run()
        {
            INVARIANT(_mail);

            while(!_done)
            try
            {
                m::message m;
                if(!_mail->pop_inbox(m, true))
                    continue;
                if(_done) continue;
                
                emit got_mail(m);
            }
            catch(std::exception& e)
            {
                LOG << "mail_service: error in mailbox `" << _mail->address() << "'. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "mail_service: unexpected error in mailbox `" << _mail->address() << "'. " << std::endl;
            }
        }
    }
}

