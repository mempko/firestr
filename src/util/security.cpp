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
#include "util/mencode.hpp"
#include "util/dbc.hpp"

#include <sstream>
#include <exception>

#include <botan/botan.h>
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

            INVARIANT(_k);
            INVARIANT_FALSE(_encrypted_private_key.empty());
        }

        const std::string& private_key::encrypted_private_key() const
        {
            INVARIANT(_k);
            ENSURE_FALSE(_encrypted_private_key.empty());
            return _encrypted_private_key;
        }

        const std::string& private_key::public_key() const
        {
            INVARIANT(_k);
            ENSURE_FALSE(_public_key.empty());
            return _public_key;
        }

        public_key::public_key() : 
            _ks{}, _k{}
        {}

        public_key::public_key(const std::string& key) : 
            _ks{key}
        {
            REQUIRE_FALSE(key.empty());

            b::DataSource_Memory ds{reinterpret_cast<const b::byte*>(&_ks[0]), _ks.size()};
            _k.reset(b::X509::load_key(ds));

            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key::public_key(const private_key& pkey) : 
            public_key(pkey.public_key()) 
        {
            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key::public_key(const public_key& pk) : 
            public_key(pk.key()) 
        {
            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key& public_key::operator=(const public_key& o)
        {
            if(&o == this) return *this;

            _ks = o._ks;
            b::DataSource_Memory ds{reinterpret_cast<const b::byte*>(&_ks[0]), _ks.size()};
            _k.reset(b::X509::load_key(ds));

            ENSURE(_k);
            ENSURE_NOT_EQUAL(_k, o._k);
            ENSURE_FALSE(_ks.empty());
            ENSURE_EQUAL(_ks, o._ks);
            return *this;
        }

        bool public_key::valid() const
        {
            return _k != nullptr;
        }

        const std::string& public_key::key() const
        {
            INVARIANT(_k);
            ENSURE_FALSE(_ks.empty());
            return _ks;
        }

        void encode(std::ostream& out, const private_key& k)
        {
            value v = k.encrypted_private_key();
            out << v;
        }

        private_key_ptr decode(std::istream& in, const std::string& passphrase)
        {
            value encrypted_private_key;
            in >> encrypted_private_key;

            return std::make_shared<private_key>(encrypted_private_key.as_string(), passphrase);
        }

        void encode(std::ostream& out, const public_key& k)
        {
            value v = k.key();
            out << v;
        }

        public_key decode(std::istream& in, const public_key& k)
        {
            value v;
            in >> v;
            return {v.as_string()};
        }
    }
}
