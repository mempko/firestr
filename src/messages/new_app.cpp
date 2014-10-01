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
#include "messages/new_app.hpp"
#include "util/dbc.hpp"

namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace messages
    {
        const std::string NEW_APP = "new_app";
        const std::string REQ_APP = "req_app";

        new_app::new_app(
                const std::string& id,
                const std::string& type) :
            _id{id},
            _type{type},
            _from_id{},
            _data{}
        {
        }

        new_app::new_app(
                const std::string& id,
                const std::string& type,
                const u::bytes& data) :
            _id{id},
            _type{type},
            _from_id{},
            _data{data}
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

        request_app::request_app(std::string a, std::string cid) 
            : app_address{a}, conversation_id{cid}
        { }

        request_app::request_app(const message::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, REQ_APP);

            app_address = m.meta.extra["app_addr"].as_string();
            conversation_id = m.meta.extra["conv_id"].as_string();
            from_id = m.meta.extra["from_id"].as_string();
        }

        request_app::operator message::message() const
        {
            m::message m;
            m.meta.type = REQ_APP;
            m.meta.extra["app_addr"] = app_address;
            m.meta.extra["conv_id"] = conversation_id;
            return m;
        }

    }
}
