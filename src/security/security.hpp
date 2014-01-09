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
 */
#ifndef FIRESTR_SECURITY_SEC_H
#define FIRESTR_SECURITY_SEC_H

#include <iostream>
#include <memory>

#include "util/bytes.hpp"
#include "util/thread.hpp"

namespace Botan
{
    class Private_Key; 
    class Public_Key; 
    class OctetString;
    typedef OctetString SymmetricKey; 
    class DH_PrivateKey;
}

namespace fire  
{
    namespace security 
    {
        using prv_key_ptr = std::shared_ptr<Botan::Private_Key>;
        using pub_key_ptr = std::shared_ptr<Botan::Public_Key>;

        class private_key
        {
            public:
                //creates new key using passphrase
                private_key(const std::string& passphrase);
                private_key(const std::string& encrypted_private_key, const std::string& passphrase);

            public:
                const std::string& encrypted_private_key() const;
                const std::string& public_key() const;

            public:
                util::bytes decrypt(const util::bytes&) const;

            private:
                prv_key_ptr _k;
                std::string _encrypted_private_key;
                std::string _public_key;
        };

        class public_key
        {
            public:
                public_key();
                public_key(const std::string& key);
                public_key(const private_key& pkey);
                public_key(const public_key&);
            public:
                public_key& operator=(const public_key&);

            public:
                bool valid() const;
                const std::string& key() const;

            public:
                util::bytes encrypt(const util::bytes&) const;

            private:
                void set(const std::string& key);

            private:
                std::string _ks;
                pub_key_ptr _k;
        };

        using private_key_ptr = std::shared_ptr<private_key>;
        using public_key_ptr = std::shared_ptr<public_key>;

        void encode(std::ostream& out, const private_key& u);
        private_key_ptr decode_private_key(std::istream& in, const std::string& passphrase);

        void encode(std::ostream& out, const public_key&);
        public_key decode_public_key(std::istream& in);

        using symmetric_key_ptr = std::shared_ptr<Botan::SymmetricKey>;
        using dh_private_key_ptr = std::shared_ptr<Botan::DH_PrivateKey>;

        class dh_secret
        {
            public:
                dh_secret();
                dh_secret(const dh_secret&);
                dh_secret& operator=(const dh_secret&);

            public:
                const util::bytes& public_value() const;
                const util::bytes& other_public_value() const;
                void create_symmetric_key(const util::bytes& public_val);

            public:
                bool ready() const;
                util::bytes encrypt(const util::bytes&) const;
                util::bytes decrypt(const util::bytes&) const;

            private:
                dh_private_key_ptr _pkey;
                symmetric_key_ptr _skey;
                util::bytes _pub_value;
                util::bytes _other_pub_value;
                mutable std::mutex _mutex;
        };

        /**
         * Randomizes the byte array with the size specified
         */
        void randomize(util::bytes&);
    }
}

#endif

