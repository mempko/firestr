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
#ifndef FIRESTR_MESSAGES_GREET_REGISTER_H
#define FIRESTR_MESSAGES_GREET_REGISTER_H

#include "message/message.hpp"
#include "util/bytes.hpp"

#include <string>

namespace fire
{
    namespace messages
    {
        extern const std::string GREET_REGISTER;
        extern const std::string GREET_FIND_REQUEST;
        extern const std::string GREET_FIND_RESPONSE;

        class greet_register
        {
            public:
                greet_register(
                        const std::string& id,
                        const std::string& ip,
                        const std::string& port);

            public:
                greet_register(const message::message&);
                operator message::message() const;

            public:
                const std::string& id() const;
                const std::string& ip() const;
                const std::string& port() const;

            private:
                std::string _id;
                std::string _ip;
                std::string _port;
        };

        class greet_find_request
        {
            public:
                greet_find_request(
                        const std::string& from_id,
                        const std::string& search_id,
                        const std::string& response_service_address);

            public:
                greet_find_request(const message::message&);
                operator message::message() const;

            public:
                const std::string& from_id() const;
                const std::string& search_id() const;
                const std::string& response_service_address() const;

            private:
                std::string _from_id;
                std::string _search_id;
                std::string _response_service_address;
        };

        class greet_find_response
        {
            public:
                greet_find_response(
                        bool found,
                        const std::string& id,
                        const std::string& ip,
                        const std::string& port);

            public:
                greet_find_response(const message::message&);
                operator message::message() const;

            public:
                bool found() const;
                const std::string& id() const;
                const std::string& ip() const;
                const std::string& port() const;

            private:
                bool _found;
                std::string _id;
                std::string _ip;
                std::string _port;
        };
    }
}

#endif
