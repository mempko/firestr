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
#ifndef FIRESTR_QUEUE_H
#define FIRESTR_QUEUE_H

#include <vector>
#include <string>

namespace fire 
{
    namespace util 
    {
        typedef char byte;
        typedef unsigned char ubyte;
        typedef std::vector<byte> bytes;
        typedef std::vector<ubyte> ubytes;

        bytes to_bytes(const std::string&);
        std::string to_str(const bytes&);
    }
}

#endif
