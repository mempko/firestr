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
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
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

        mailbox::~mailbox()
        {
            done();
        }

        void mailbox::done()
        {
            _in.done();
            _out.done();
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

        bool mailbox::pop_inbox(message& m, bool wait)
        {
            const bool p = _in.pop(m, wait);
            if(_stats.on && p) _stats.in_pop_count++;
            return p;
        }

        void mailbox::push_outbox(const message& m)
        {
            if(_stats.on) _stats.out_push_count++;
            _out.push(m);
        }

        bool mailbox::pop_outbox(message& m, bool wait)
        {
            bool p = _out.pop(m, wait);
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
