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
#ifndef FIRESTR_MESSAGES_NEW_APP_H
#define FIRESTR_MESSAGES_NEW_APP_H

#include "message/message.hpp"
#include "util/bytes.hpp"

#include <string>

namespace fire
{
    namespace messages
    {
        extern const std::string NEW_APP;

        class new_app
        {
            public:
                new_app();
                new_app(const std::string& id, const std::string& type);
                new_app(const std::string& id, const std::string& type, const util::bytes& data);

            public:
                new_app(const message::message&);
                operator message::message() const;

            public:
                const std::string& id() const;
                const std::string& type() const;
                const std::string& from_id() const;
                const util::bytes& data() const;

            private:
                std::string _id;
                std::string _type;
                std::string _from_id;
                util::bytes _data;
        };
    }
}

#endif
