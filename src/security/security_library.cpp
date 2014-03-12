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
#include "security/security_library.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace u = fire::util;

namespace fire 
{
    namespace security 
    {
        encrypted_channels::encrypted_channels(const private_key& pk) : _pk(pk) {}

        u::bytes append_prefix(char p, const u::bytes& bs)
        {
            u::bytes rs;
            rs.reserve(bs.size() + 1);
            rs.push_back(p);
            rs.insert(rs.end(), bs.begin(), bs.end());
            return rs;
        }

        u::bytes encrypted_channels::encrypt_asymmetric(channel_map::const_iterator s, const u::bytes& bs) const
        {
            if(bs.empty()) return {};

            if(s == _s.end()) return {};

            auto es = s->second.key.encrypt(bs);
            return append_prefix(encryption_type::asymmetric, es);
        }

        u::bytes encrypted_channels::encrypt_asymmetric(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            return encrypt_asymmetric(_s.find(i), bs);
        }


        u::bytes encrypted_channels::encrypt_plaintext(const u::bytes& bs) const
        {
            return append_prefix(encryption_type::plaintext, bs);
        }

        u::bytes encrypted_channels::encrypt_symmetric(channel_map::const_iterator s, const u::bytes& bs) const
        {
            if(s == _s.end()) return {};
            REQUIRE(s->second.shared_secret.ready());

            auto es = s->second.shared_secret.encrypt(bs);
            return append_prefix(encryption_type::symmetric, es);
        }

        u::bytes encrypted_channels::encrypt_symmetric(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            return encrypt_symmetric(_s.find(i), bs);
        }

        u::bytes encrypted_channels::encrypt(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            auto s = _s.find(i);
            if(s == _s.end()) return encrypt_plaintext(bs); 

            if(!s->second.shared_secret.ready())
            {
                return encrypt_asymmetric(s, bs);
            }

            return encrypt_symmetric(s, bs);
        }

        u::bytes encrypted_channels::decrypt(const id& i, const u::bytes& bs, encryption_type& et) const
        {
            if(bs.size() < 2) return {};

            u::bytes ds;
            auto message_start = bs.begin();
            message_start++;

            switch(bs[0])
            {
                case encryption_type::plaintext: 
                    {
                        et = encryption_type::plaintext;
                        ds.reserve(bs.size()-1); 
                        auto b = bs.begin(); b++;
                        ds.insert(ds.begin(), b, bs.end());
                    }
                    break;
                case encryption_type::symmetric: 
                    {
                        u::mutex_scoped_lock l(_mutex);
                        et = encryption_type::symmetric;
                        auto s = _s.find(i);
                        if(s == _s.end()) return {};
                        if(!s->second.shared_secret.ready()) return {};
                        u::bytes cb{message_start, bs.end()};
                        ds = s->second.shared_secret.decrypt(cb);
                    }
                    break;
                case encryption_type::asymmetric: 
                    {
                        et = encryption_type::asymmetric;
                        //decrypt message, skipping encryption type prefix
                        u::bytes cb{message_start, bs.end()};
                        ds = _pk.decrypt(cb);
                    }
                    break;
                default: 
                    {
                        et = encryption_type::unknown;
                        return {};
                    }
            }
            return ds;
        }

        void encrypted_channels::create_channel(const id& i, const public_key& key)
        {
            REQUIRE(key.valid());

            u::mutex_scoped_lock l(_mutex);

            auto& s = _s[i];
            if(s.key.valid() && s.key.key() == key.key()) return;

            LOG << "creating pk security channel for: " << i << std::endl;

            s.key = key;
            s.shared_secret = dh_secret{};

            ENSURE(s.key.valid());
        }

        void encrypted_channels::create_channel(const id& i, const public_key& key, const util::bytes& public_val)
        {
            REQUIRE(key.valid());
            u::mutex_scoped_lock l(_mutex);

            LOG << "creating pk/dh security channel for: " << i << std::endl;

            auto& s = _s[i];

            //update public key if changed
            if(!s.key.valid() || s.key.key() != key.key()) s.key = key;

            s.shared_secret.create_symmetric_key(public_val);

            ENSURE(s.key.valid());
            ENSURE(s.shared_secret.ready());
        }

        const channel& encrypted_channels::get_channel(const id& i) const
        {
            u::mutex_scoped_lock l(_mutex);
            auto r = _s.find(i);
            REQUIRE(r != _s.end());

            return r->second;
        }

        void encrypted_channels::remove_channel(const id& i)
        {
            u::mutex_scoped_lock l(_mutex);
            if(!_s.count(i)) return;

            _s.erase(i);
        }
    }
}
