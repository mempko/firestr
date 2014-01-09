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
#include "messages/new_app.hpp"
#include "util/dbc.hpp"

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace messages
    {
        const std::string NEW_APP = "new_app";

        new_app::new_app(
                const std::string& id,
                const std::string& type) :
            _id{id},
            _type{type},
            _data{},
            _from_id{}
        {
        }

        new_app::new_app(
                const std::string& id,
                const std::string& type,
                const u::bytes& data) :
            _id{id},
            _type{type},
            _data{data},
            _from_id{}
        {
        }

        new_app::new_app(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, NEW_APP);

            _id = m.meta.extra["app_id"].as_string();
            _type = m.meta.extra["app_type"].as_string();
            _from_id = m.meta.extra["from_id"].as_string();
            _data = m.data;
        }

        new_app::operator message::message() const
        {
            m::message m;
            m.meta.type = NEW_APP;
            m.meta.extra["app_id"] = _id;
            m.meta.extra["app_type"] = _type;
            m.data = _data;
            return m;
        }

        const std::string& new_app::id() const
        {
            return _id;
        }

        const std::string& new_app::type() const
        {
            return _type;
        }

        const u::bytes& new_app::data() const
        {
            return _data;
        }

        const std::string& new_app::from_id() const
        {
            return _from_id;
        }
    }
}
