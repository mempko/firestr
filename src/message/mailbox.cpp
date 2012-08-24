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
#include "message/mailbox.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace message
    {
        mailbox::mailbox() : 
            _address{}, _in{}, _out{}
        {
        }

        mailbox::mailbox(const std::string& a) : 
            _address{a}, _in{}, _out{}
        {
        }

        const std::string& mailbox::address() const
        {
            return _address;
        }

        void mailbox::address(const std::string& a)
        {
            _address = a;
        }

        void mailbox::push_inbox(const message& m)
        {
            _in.push(m);
            ENSURE_GREATER(_in.size(), 0);
        }

        bool mailbox::pop_inbox(message& m)
        {
            return _in.pop(m);
        }

        void mailbox::push_outbox(const message& m)
        {
            _out.push(m);
            ENSURE_GREATER(_out.size(), 0);
        }

        bool mailbox::pop_outbox(message& m)
        {
            return _out.pop(m);
        }
    }
}
