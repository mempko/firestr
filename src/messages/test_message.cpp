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
#include "messages/test_message.hpp"
#include "util/dbc.hpp"

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace messages
    {
        const std::string TEST_MESSAGE = "test";

        test_message::test_message() : _text{}, _from_id{} { }

        test_message::test_message(const std::string& text) :
            _text{text},
            _from_id{}
        {
        }

        test_message::test_message(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, TEST_MESSAGE);

            _text = u::to_str(m.data);
            _from_id = m.meta.extra["from_id"].as_string();
        }

        test_message::operator message::message() const
        {
            m::message m;
            m.meta.type = TEST_MESSAGE;
            m.meta.extra["id"] = _from_id;
            m.data = u::to_bytes(_text);
            return m;
        }

        const std::string& test_message::text() const
        {
            return _text;
        }

        const std::string& test_message::from_id() const
        {
            return _from_id;
        }
    }
}
