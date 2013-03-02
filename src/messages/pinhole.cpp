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
#include "messages/pinhole.hpp"
#include "util/dbc.hpp"

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace messages
    {
        const std::string PINHOLE = "pinhole";

        pinhole::pinhole() {}

        pinhole::pinhole(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, PINHOLE);
        }

        pinhole::operator message::message() const
        {
            m::message m;
            m.meta.type = PINHOLE;
            return m;
        }
    }
}
