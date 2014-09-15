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
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */
#include "messages/greeter.hpp"
#include "util/dbc.hpp"

namespace n = fire::network;
namespace m = fire::message;
namespace u = fire::util;

namespace fire
{
    namespace messages
    {
        const std::string GREET_KEY_REQUEST = "greet_key_request";
        const std::string GREET_KEY_RESPONSE = "greet_key_response";
        const std::string GREET_REGISTER = "greet_register";
        const std::string GREET_FIND_REQUEST = "greet_request";
        const std::string GREET_FIND_RESPONSE = "greet_response";

        greet_key_request::greet_key_request( const std::string& response_service_address) :
            _response_service_address{response_service_address} { }

        greet_key_request::greet_key_request(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, GREET_KEY_REQUEST);

            _response_service_address = m.meta.extra["response_address"].as_string();
        }

        greet_key_request::operator message::message() const
        {
            m::message m;
            m.meta.type = GREET_KEY_REQUEST;
            m.meta.extra["response_address"] = _response_service_address;
            return m;
        }

        const std::string& greet_key_request::response_service_address() const
        {
            return _response_service_address;
        }

        greet_key_response::greet_key_response(const std::string& key) :
            _pub_key{key}, _host{}, _port{} {}

        greet_key_response::greet_key_response(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, GREET_KEY_RESPONSE);
            _pub_key = m.meta.extra["pub_key"].as_string();
            _host = m.meta.extra["from_ip"].as_string();
            _port = m.meta.extra["from_port"].as_int();
        }

        greet_key_response::operator message::message() const
        {
            m::message m;
            m.meta.type = GREET_KEY_RESPONSE;
            m.meta.extra["pub_key"] = _pub_key;
            return m;
        }

        const std::string& greet_key_response::host() const
        {
            return _host;
        }

        n::port_type greet_key_response::port() const
        {
            return _port;
        }

        const std::string& greet_key_response::key() const
        {
            return _pub_key;
        }

        greet_register::greet_register(
                const std::string& id,
                const greet_endpoint& local,
                const std::string& pbkey,
                const std::string& response_service_address) :
            _id{id},
            _local(local),
            _pub_key(pbkey),
            _response_service_address{response_service_address}
        {
        }

        greet_register::greet_register(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, GREET_REGISTER);

            _id = m.meta.extra["from_id"].as_string();
            _local.ip = m.meta.extra["loc_ip"].as_string();
            _local.port = m.meta.extra["loc_port"].as_int();
            _pub_key = m.meta.extra["pub_key"].as_string();
            _response_service_address = m.meta.extra["response_address"].as_string();
        }

        greet_register::operator message::message() const
        {
            m::message m;
            m.meta.type = GREET_REGISTER;
            m.meta.extra["from_id"] = _id;
            m.meta.extra["loc_ip"] = _local.ip;
            m.meta.extra["loc_port"] = static_cast<int>(_local.port);
            m.meta.extra["pub_key"] = _pub_key;
            m.meta.extra["response_address"] = _response_service_address;
            return m;
        }

        const std::string& greet_register::id() const
        {
            return _id;
        }

        const greet_endpoint& greet_register::local() const
        {
            return _local;
        }

        const std::string& greet_register::pub_key() const
        {
            return _pub_key;
        }

        const std::string& greet_register::response_service_address() const
        {
            return _response_service_address;
        }

        greet_find_request::greet_find_request(
                const std::string& from_id,
                const std::string& search_id) :
            _from_id{from_id},
            _search_id{search_id}
        {
        }

        greet_find_request::greet_find_request(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, GREET_FIND_REQUEST);

            _from_id = m.meta.extra["from_id"].as_string();
            _search_id = m.meta.extra["search_id"].as_string();
        }

        greet_find_request::operator message::message() const
        {
            m::message m;
            m.meta.type = GREET_FIND_REQUEST;
            m.meta.extra["from_id"] = _from_id;
            m.meta.extra["search_id"] = _search_id;
            return m;
        }

        const std::string& greet_find_request::from_id() const
        {
            return _from_id;
        }

        const std::string& greet_find_request::search_id() const
        {
            return _search_id;
        }

        greet_find_response::greet_find_response(
                bool found,
                const std::string& id,
                const greet_endpoint& local,
                const greet_endpoint& ext) :
            _found{found},
            _id{id},
            _local(local),
            _ext(ext)
        {
        }

        greet_find_response::greet_find_response(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, GREET_FIND_RESPONSE);

            _id = m.meta.extra["search_id"].as_string();
            _local.ip = m.meta.extra["loc_ip"].as_string();
            _local.port = m.meta.extra["loc_port"].as_int();
            _ext.ip = m.meta.extra["ext_ip"].as_string();
            _ext.port = m.meta.extra["ext_port"].as_int();
            _found = m.meta.extra["found"].as_int() == 1;
        }

        greet_find_response::operator message::message() const
        {
            m::message m;
            m.meta.type = GREET_FIND_RESPONSE;
            m.meta.extra["search_id"] = _id;
            m.meta.extra["loc_ip"] = _local.ip;
            m.meta.extra["loc_port"] = static_cast<int>(_local.port);
            m.meta.extra["ext_ip"] = _ext.ip;
            m.meta.extra["ext_port"] = static_cast<int>(_ext.port);
            m.meta.extra["found"] = static_cast<int>(_found ? 1 : 0);
            return m;
        }

        
        bool greet_find_response::found() const
        {
            return _found;
        }
        
        const std::string& greet_find_response::id() const
        {
            return _id;
        }

        const greet_endpoint& greet_find_response::local() const
        {
            return _local;
        }

        const greet_endpoint& greet_find_response::external() const
        {
            return _ext;
        }

    }
}
