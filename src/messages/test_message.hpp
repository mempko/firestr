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
#ifndef FIRESTR_MESSAGES_TEST_MESSAGE_H
#define FIRESTR_MESSAGES_TEST_MESSAGE_H

#include "messages/sender.hpp"

#include <string>

namespace fire
{
    namespace messages
    {
        extern const std::string TEST_MESSAGE;

        class test_message
        {
            public:
                test_message();
                test_message(const std::string& text);
                test_message(const message::message&);
                operator message::message() const;
            public:
                const std::string& text() const;
                const std::string& from_id() const;

            private:
                std::string _text;
                std::string _from_id;
        };
    }
}

#endif
