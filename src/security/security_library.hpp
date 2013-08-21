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
#ifndef FIRESTR_SECURITY_LIBRARY_H
#define FIRESTR_SECURITY_LIBRARY_H

#include <iostream>
#include <memory>
#include <unordered_map>

#include "security/security.hpp"
#include "util/thread.hpp"

namespace fire  
{
    namespace security 
    {
        using id = std::string;
        using shared_secret = std::string;

        struct session
        {
            shared_secret secret;
            public_key key;
        };

        using session_map = std::unordered_map<id, session>;

        class session_library
        {
            public:
                session_library(const private_key&);

            public:
                util::bytes encrypt(const id&, const util::bytes&) const;
                util::bytes encrypt_assymetric(const id&, const util::bytes&) const;
                util::bytes encrypt_symmetric(const id&, const util::bytes&) const;
                util::bytes encrypt_plaintext(const util::bytes&) const;

                util::bytes decrypt(const id&, const util::bytes&) const;

            public:
                void add_session(const id&, const session&);
                void remove_session(const id&);

            private:
                util::bytes encrypt_assymetric(session_map::const_iterator, const util::bytes&) const;
                util::bytes encrypt_symmetric(session_map::const_iterator, const util::bytes&) const;

            private:
                session_map _s;
                const private_key& _pk;
                mutable std::mutex _mutex;
        };

        using session_library_ptr = std::shared_ptr<session_library>;
    }
}

#endif
