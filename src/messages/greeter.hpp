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
#ifndef FIRESTR_MESSAGES_GREET_REGISTER_H
#define FIRESTR_MESSAGES_GREET_REGISTER_H

#include "network/endpoint.hpp"
#include "message/message.hpp"
#include "util/bytes.hpp"

#include <string>

namespace fire
{
    namespace messages
    {
        extern const std::string GREET_KEY_REQUEST;
        extern const std::string GREET_KEY_RESPONSE;
        extern const std::string GREET_REGISTER;
        extern const std::string GREET_FIND_REQUEST;
        extern const std::string GREET_FIND_RESPONSE;

        struct greet_endpoint
        {
            std::string ip;
            network::port_type port;

            bool operator==(const greet_endpoint& o) const
            {
                return ip == o.ip && port == o.port;
            }

            bool operator!=(const greet_endpoint& o) const
            {
                return !(operator==(o));
            }
        };

        class greet_key_request
        {
            public:
                greet_key_request(const std::string& response_service_address);

            public:
                greet_key_request(const message::message&);
                operator message::message() const;

            public:
                const std::string& response_service_address() const;

            private:
                std::string _response_service_address;
        };

        class greet_key_response
        {
            public:
                greet_key_response(const std::string& key);

            public:
                greet_key_response(const message::message&);
                operator message::message() const;

            public:
                const std::string& host() const;
                network::port_type port() const;
                const std::string& key() const;

            private:
                std::string _host;
                network::port_type _port;
                std::string _pub_key;
        };

        class greet_register
        {
            public:
                greet_register(
                        const std::string& id,
                        const greet_endpoint& local,
                        const std::string& pbkey,
                        const std::string& response_service_address);

            public:
                greet_register(const message::message&);
                operator message::message() const;

            public:
                const std::string& id() const;
                const greet_endpoint& local() const;
                const std::string& pub_key() const;
                const std::string& response_service_address() const;

            private:
                std::string _id;
                greet_endpoint _local;
                std::string _pub_key;
                std::string _response_service_address;
        };

        class greet_find_request
        {
            public:
                greet_find_request(
                        const std::string& from_id,
                        const std::string& search_id);

            public:
                greet_find_request(const message::message&);
                operator message::message() const;

            public:
                const std::string& from_id() const;
                const std::string& search_id() const;

            private:
                std::string _from_id;
                std::string _search_id;
        };

        class greet_find_response
        {
            public:
                greet_find_response(
                        bool found,
                        const std::string& id,
                        const greet_endpoint& local,
                        const greet_endpoint& ext);

            public:
                greet_find_response(const message::message&);
                operator message::message() const;

            public:
                bool found() const;
                const std::string& id() const;
                const greet_endpoint& local() const;
                const greet_endpoint& external() const;

            private:
                bool _found;
                std::string _id;
                greet_endpoint _local;
                greet_endpoint _ext;
        };
    }
}

#endif
