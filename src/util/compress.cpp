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
#include "util/compress.hpp"
#include <snappy.h>

namespace sn = snappy;

namespace fire 
{
    namespace util 
    {
        bytes compress(const bytes& i)
        {
            std::string o;
            sn::Compress(i.data(), i.size(), &o);
            return to_bytes(o);
        }

        bytes uncompress(const bytes& i)
        {
            std::string o;
            if(!sn::Uncompress(i.data(), i.size(), &o)) return {};
            return to_bytes(o);
        }
    }
}

