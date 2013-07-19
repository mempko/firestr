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
#ifndef FIRESTR_UTIL_SEC_H
#define FIRESTR_UTIL_SEC_H

#include <iostream>
#include <memory>

namespace Botan
{
    class Private_Key; 
    class Public_Key; 
}

namespace fire  
{
    namespace util 
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

            private:
                std::string _ks;
                pub_key_ptr _k;
        };

        using private_key_ptr = std::shared_ptr<private_key>;
        using public_key_ptr = std::shared_ptr<public_key>;

        void encode(std::ostream& out, const private_key& u);
        private_key_ptr decode(std::istream& in, const std::string& passphrase);

        void encode(std::ostream& out, const public_key&);
        public_key decode(std::istream& in, const public_key&);
    }
}

#endif

