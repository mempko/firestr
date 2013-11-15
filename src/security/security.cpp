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
#include "security/security.hpp"
#include "util/mencode.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <sstream>
#include <exception>

#include <botan/botan.h>

#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/look_pk.h>
#include <botan/dh.h>

namespace b = Botan;
namespace u = fire::util;

namespace fire 
{
    namespace security 
    {
        namespace
        {
            const size_t RSA_SIZE = 1024;
            const std::string EME_SCHEME = "EME1(SHA-256)";
            const std::string KEY_AGREEMENT_ALGO = "KDF2(SHA-256)";
            const std::string SESSION_PARAM = "firestr";
            const std::string CYPHER = "AES-128/CBC";
            const std::string SHARED_DOMAIN = "modp/ietf/2048";
            std::mutex BOTAN_MUTEX;
        }

        void validate_passphrase(const std::string& passphrase)
        {
            if(passphrase.size() > 50)
                throw std::invalid_argument{"Password must be less than 50 characters"};
        }

        private_key::private_key(const std::string& passphrase)
        {
            u::mutex_scoped_lock l(BOTAN_MUTEX);
            validate_passphrase(passphrase);

            b::AutoSeeded_RNG r;

            _k.reset(new b::RSA_PrivateKey{r, RSA_SIZE});
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
            u::mutex_scoped_lock l(BOTAN_MUTEX);

            validate_passphrase(passphrase);

            b::AutoSeeded_RNG r;
            b::DataSource_Memory ds{
                reinterpret_cast<const b::byte*>(&_encrypted_private_key[0]), 
                    _encrypted_private_key.size()};

            _k.reset(b::PKCS8::load_key(ds, r, passphrase));
            _public_key = b::X509::PEM_encode(*_k);

            if(!_k) throw std::invalid_argument{"Invalid Password"};

            INVARIANT(_k);
            INVARIANT_FALSE(_encrypted_private_key.empty());
            ENSURE_FALSE(_public_key.empty());
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

        void public_key::set(const std::string& key) 
        {
            REQUIRE_FALSE(key.empty());
            u::mutex_scoped_lock l(BOTAN_MUTEX);

            b::DataSource_Memory ds{reinterpret_cast<const b::byte*>(&_ks[0]), _ks.size()};
            _k.reset(b::X509::load_key(ds));

            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key::public_key(const std::string& key) : 
            _ks{key}
        {
            REQUIRE_FALSE(key.empty());

            set(key);

            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key::public_key(const private_key& pkey) : 
            public_key(pkey.public_key()) 
        {
            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key::public_key(const public_key& pk) : _k{}, _ks{pk._ks}
        {
            if(!pk._k) return;

            set(pk.key());

            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
        }

        public_key& public_key::operator=(const public_key& o)
        {
            if(&o == this) return *this;
            u::mutex_scoped_lock l(BOTAN_MUTEX);

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
            u::value v = k.encrypted_private_key();
            out << v;
        }

        private_key_ptr decode_private_key(std::istream& in, const std::string& passphrase)
        {
            u::value encrypted_private_key;
            in >> encrypted_private_key;

            return std::make_shared<private_key>(encrypted_private_key.as_string(), passphrase);
        }

        void encode(std::ostream& out, const public_key& k)
        {
            u::value v = k.key();
            out << v;
        }

        public_key decode_public_key(std::istream& in)
        {
            u::value v;
            in >> v;
            return {v.as_string()};
        }

        u::bytes private_key::decrypt(const u::bytes& b) const
        {
            INVARIANT(_k);
            u::mutex_scoped_lock l(BOTAN_MUTEX);

            b::PK_Decryptor_EME d{*_k, EME_SCHEME};

            u::bytes rs;
            std::stringstream s(u::to_str(b));
            u::bytes bs;
            s >> bs;
            while(!bs.empty())
            {
                auto r = d.decrypt(reinterpret_cast<const unsigned char*>(bs.data()), bs.size());
                rs.insert(rs.end(), std::begin(r), std::end(r));

                s >> bs;
            }
            return rs;
        }

        u::bytes public_key::encrypt(const u::bytes& b) const
        {
            INVARIANT(_k);
            INVARIANT_FALSE(_ks.empty());
            u::mutex_scoped_lock l(BOTAN_MUTEX);

            std::stringstream rs;

            b::PK_Encryptor_EME e{*_k, EME_SCHEME};

            b::AutoSeeded_RNG r;
            size_t advance = 0;
            while(advance < b.size())
            {
                size_t size = std::min(e.maximum_input_size(), b.size()-advance);
                auto c = e.encrypt(reinterpret_cast<const unsigned char*>(b.data())+advance, size, r);
                u::bytes bs{std::begin(c), std::end(c)};
                rs << bs;
                advance+=size;
            }
            return u::to_bytes(rs.str());
        }

        dh_secret::dh_secret()
        {
            u::mutex_scoped_lock l(BOTAN_MUTEX);
            b::AutoSeeded_RNG r;
            b::DL_Group sd{SHARED_DOMAIN};
            _pkey = std::make_shared<b::DH_PrivateKey>(r, sd);
            auto p = _pkey->public_value();
            _pub_value = u::bytes{std::begin(p), std::end(p)};
            ENSURE(_pkey);
            ENSURE_FALSE(_pub_value.empty());
        }

        dh_secret::dh_secret(const dh_secret& o) : 
            _pkey{o._pkey}, _skey{o._skey}, 
            _pub_value{o._pub_value}, 
            _other_pub_value{o._other_pub_value} {}

        dh_secret& dh_secret::operator=(const dh_secret& o)
        {
            if(&o == this) return *this;
            u::mutex_scoped_lock l(_mutex);
            _pkey = o._pkey;
            _skey = o._skey;
            _pub_value = o._pub_value;
            _other_pub_value = o._other_pub_value;
            return *this;
        }

        const util::bytes& dh_secret::public_value() const
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT_FALSE(_pub_value.empty());
            return _pub_value;
        }

        const util::bytes& dh_secret::other_public_value() const
        {
            u::mutex_scoped_lock l(_mutex);
            ENSURE( _skey == nullptr || !_other_pub_value.empty());
            return _other_pub_value;
        }

        void dh_secret::create_symmetric_key(const util::bytes& pv)
        {
            u::mutex_scoped_lock bl(BOTAN_MUTEX);
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_pkey);

            b::PK_Key_Agreement k{*_pkey, KEY_AGREEMENT_ALGO};
            _skey = 
                std::make_shared<b::SymmetricKey>(
                        k.derive_key(
                            16, 
                            reinterpret_cast<const unsigned char*>(pv.data()), pv.size(),
                            SESSION_PARAM));

            _other_pub_value = pv;
            ENSURE(_skey);
        }

        bool dh_secret::ready() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _skey != nullptr;
        }

        util::bytes dh_secret::encrypt(const util::bytes& bs) const
        {
            REQUIRE(ready());
            u::mutex_scoped_lock bl(BOTAN_MUTEX);
            u::mutex_scoped_lock l(_mutex);
            b::Pipe p{b::get_cipher(CYPHER, *_skey, b::ENCRYPTION)};
            p.start_msg();
            p.write(reinterpret_cast<const unsigned char*>(bs.data()), bs.size());
            p.end_msg();
              
            auto e = p.read_all(0);
            return {std::begin(e), std::end(e)};
        }

        util::bytes dh_secret::decrypt(const util::bytes& bs) const
        {
            REQUIRE(ready());
            u::mutex_scoped_lock bl(BOTAN_MUTEX);
            u::mutex_scoped_lock l(_mutex);
            b::Pipe p{b::get_cipher(CYPHER, *_skey, b::DECRYPTION)};
            p.start_msg();
            p.write(reinterpret_cast<const unsigned char*>(bs.data()), bs.size());
            p.end_msg();
            auto e = p.read_all(0);
            return {std::begin(e), std::end(e)};
        }
    }
}
