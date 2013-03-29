/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
        mailbox_stats::mailbox_stats() :
            in_push_count{0},
            out_push_count{0},
            in_pop_count{0},
            out_pop_count{0},
            on{false}
        {
        }


        void mailbox_stats::reset()
        {
            in_push_count = 0;
            out_push_count = 0;
            in_pop_count = 0;
            out_pop_count = 0;
        }

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
            if(_stats.on) _stats.in_push_count++;
            _in.push(m);
        }

        bool mailbox::pop_inbox(message& m)
        {
            const bool p = _in.pop(m);
            if(_stats.on && p) _stats.in_pop_count++;
            return p;
        }

        void mailbox::push_outbox(const message& m)
        {
            if(_stats.on) _stats.out_push_count++;
            _out.push(m);
        }

        bool mailbox::pop_outbox(message& m)
        {
            bool p = _out.pop(m);
            if(_stats.on && p) _stats.out_pop_count++;
            return p;
        }

        size_t mailbox::in_size() const
        {
            return _in.size();
        }

        size_t mailbox::out_size() const
        {
            return _out.size();
        }

        const mailbox_stats& mailbox::stats() const
        {
            return _stats;
        }

        mailbox_stats& mailbox::stats() 
        {
            return _stats;
        }

        void mailbox::stats(bool on)
        {
            _stats.on = on;
        }
    }
}
