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
#ifndef FIRESTR_MESSAGE_MESSAGE_H
#define FIRESTR_MESSAGE_MESSAGE_H

#include <string>
#include <iostream>
#include <deque>

#include "util/mencode.hpp"
#include "util/bytes.hpp"

namespace fire
{
    namespace message
    {
        typedef std::deque<std::string> address;
        struct metadata
        {
            std::string type;
            address to;
            address from;
            util::dict extra;
        };

        struct message
        {
            metadata meta; 
            util::bytes data;
        };

        std::ostream& operator<<(std::ostream&, const message&);
        std::istream& operator>>(std::istream&, message&);

    }

}

namespace std
{
    std::ostream& operator<<(std::ostream&, fire::message::address);
}
#endif 
