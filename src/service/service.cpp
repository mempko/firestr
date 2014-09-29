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

                s->message_received(m);
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

        void service::handle(const std::string& t, message_handler h)
        {
            _h[t] = h;
        }

        void service::message_received(const message::message& m)
        {
            //find handler
            auto h = _h.find(m.meta.type);
            if(h == _h.end()) 
            {
                LOG << "error, no handler found for`" << m.meta.type << "' in " << _address << std::endl;
                return;
            }

            //call handler
            (h->second)(m);
        }
    }
}
