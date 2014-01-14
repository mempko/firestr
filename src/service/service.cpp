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

#include "service/service.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <stdexcept>

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace service
    {
        void service_thread(service* s)
        try
        {
            REQUIRE(s);
            REQUIRE(s->_mail);

            while(!s->_done)
            try
            {
                m::message m;
                if(!s->_mail->pop_inbox(m, true))
                    continue;

                s->message_recieved(m);
            }
            catch(std::exception& e)
            {
                LOG << "Error recieving message for mailbox " << s->_address << ". " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "Unknown error recieving message for mailbox " << s->_address << std::endl;
            }
        }
        catch(...)
        {
            LOG << "exit: service::service_thread" << std::endl;
        }


        service::service(
                const std::string& address, 
                message::mailbox_ptr event) :
            _address{address},
            _event{event},
            _done{false}
        {
            REQUIRE_FALSE(address.empty());

            _mail = std::make_shared<m::mailbox>(_address);

            //start user thread
            _thread.reset(new std::thread{service_thread, this});

            INVARIANT(_mail);
            INVARIANT(_thread);
        }

        service::~service()
        {
            INVARIANT(_thread);
            INVARIANT(_mail);

            _done = true;
            _mail->done();
            _thread->join();
        }

        message::mailbox_ptr service::mail()
        {
            ENSURE(_mail);
            return _mail;
        }

        void service::send_event(const message::message& e)
        {
            if(!_event) return;
            _event->push_inbox(e);
        }
    }
}
