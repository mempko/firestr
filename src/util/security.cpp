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
#include "util/security.hpp"
#include "util/dbc.hpp"

#include <sstream>
#include <exception>

#include <botan/dsa.h>
#include <botan/rng.h>

namespace b = Botan;

namespace fire 
{
    namespace util 
    {
        namespace
        {
            const std::string DL_GROUP = "dsa/jce/1024";
        }

        void validate_passphrase(const std::string& passphrase)
        {
            if(passphrase.size() > 50)
                throw std::invalid_argument{"Password must be less than 50 characters"};
        }

        private_key::private_key(const std::string& passphrase)
        {
            validate_passphrase(passphrase);

            b::AutoSeeded_RNG r;
            b::DL_Group g{DL_GROUP};

            _k.reset(new b::DSA_PrivateKey{r, g});
            _public_key = b::X509::PEM_encode(*_k);
            _encrypted_private_key = b::PKCS8::PEM_encode(*_k, r, passphrase);

            ENSURE(_k);
            ENSURE_FALSE(_encrypted_private_key.empty());
            ENSURE_FALSE(_public_key.empty());
        }

        private_key::private_key(
                const std::string& encrypted_private_key, 
                const std::string& passphrase) :
            _encrypted_private_key{encrypted_private_key}
        {
            REQUIRE_FALSE(encrypted_private_key.empty());

            validate_passphrase(passphrase);

            b::AutoSeeded_RNG r;
            b::DataSource_Memory ds{
                reinterpret_cast<const b::byte*>(&_encrypted_private_key[0]), 
                    _encrypted_private_key.size()};

            _k.reset(b::PKCS8::load_key(ds, r, passphrase));

            if(!_k) throw std::invalid_argument{"Invalid Password"};

            ENSURE(_k);
            ENSURE_FALSE(_encrypted_private_key.empty());
        }

        const std::string& private_key::encrypted_private_key() const
        {
            ENSURE_FALSE(_encrypted_private_key.empty());
            return _encrypted_private_key;
        }

        const std::string& private_key::public_key() const
        {
            return _public_key;
        }

        public_key::public_key(const std::string& key) : 
            _ks{key}
        {
            REQUIRE_FALSE(_ks.empty());
            //TODO: creat key from string 
        }

        public_key::public_key(const private_key& pkey) : 
            public_key(pkey.public_key()) {}

        const std::string& public_key::key() const
        {
            ENSURE_FALSE(_ks.empty());
            return _ks;
        }
    }
}
