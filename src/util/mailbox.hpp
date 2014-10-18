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
#ifndef FIRESTR_UTIL_MAILBOX_H
#define FIRESTR_UTIL_MAILBOX_H

#include <string>
#include <memory>

#include "util/queue.hpp"

namespace fire
{
    namespace util
    {
        template<class letter>
        class mailbox
        {
            public:
                mailbox() : _address{}, _in{}, _out{} { }
                mailbox(const std::string& a) : _address{a}, _in{}, _out{} { }
                ~mailbox() { done(); }

            public:
                const std::string& address() const { return _address; }
                void address(const std::string& a) { _address = a; }

            public:
                void push_inbox(const letter& l) { _in.push(l); }
                bool pop_inbox(letter& l, bool wait = false) { return _in.pop(l, wait); }

            public:
                void push_outbox(const letter& l) { _out.push(l); }
                bool pop_outbox(letter& l, bool wait = false) { return _out.pop(l, wait); }

            public:
                size_t in_size() const { return _in.size(); }
                size_t out_size() const { return _out.size(); }

            public:
                void done() { _in.done(); _out.done(); }

            private:
                std::string _address;
                queue<letter> _in;
                queue<letter> _out;
        };
    }
}
#endif
