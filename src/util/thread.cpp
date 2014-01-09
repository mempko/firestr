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
#include "util/thread.hpp"
#include "util/dbc.hpp"

#include <chrono>

namespace fire
{
    namespace util
    {
        void sleep_thread(size_t ms)
        {
            REQUIRE_GREATER(ms, 0);

            std::chrono::milliseconds s(ms);
            std::this_thread::sleep_for(s);
        }
    }
}
