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

        struct channel
        {
            dh_secret shared_secret;
            public_key key;
        };

        using channel_map = std::unordered_map<id, channel>;

        enum encryption_type { plaintext='P', symmetric='S', asymmetric='A', unknown='U'};

        class encrypted_channels
        {
            public:
                encrypted_channels(const private_key&);

            public:
                util::bytes encrypt(const id&, const util::bytes&) const;
                util::bytes encrypt_asymmetric(const id&, const util::bytes&) const;
                util::bytes encrypt_symmetric(const id&, const util::bytes&) const;
                util::bytes encrypt_plaintext(const util::bytes&) const;

                util::bytes decrypt(const id&, const util::bytes&, encryption_type&) const;

            public:
                void create_channel(const id&, const public_key&);
                void create_channel(const id&, const public_key&, const util::bytes& public_val);
                const channel& get_channel(const id&) const;
                void remove_channel(const id&);

            private:
                util::bytes encrypt_asymmetric(channel_map::const_iterator, const util::bytes&) const;
                util::bytes encrypt_symmetric(channel_map::const_iterator, const util::bytes&) const;

            private:
                channel_map _s;
                const private_key& _pk;
                mutable std::mutex _mutex;
        };

        using encrypted_channels_ptr = std::shared_ptr<encrypted_channels>;
    }
}

#endif
