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

#include "util/time.hpp"

#include <sstream>
#include <chrono>

namespace fire 
{
    namespace util 
    {
        std::string timestamp()
        {
            auto t = std::chrono::system_clock::now();
            auto nt = std::chrono::system_clock::to_time_t(t);
            std::stringstream s;
            s << std::ctime(&nt);
            return s.str();
        }
    }
}


