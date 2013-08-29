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
#include "security/security_library.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace u = fire::util;

namespace fire 
{
    namespace security 
    {
        namespace
        {
            enum encryption_type { plaintext='P', symmetric='S', assymetric='A'};
        }

        session_library::session_library(const private_key& pk) : _pk(pk) {}

        u::bytes append_prefix(char p, const u::bytes& bs)
        {
            u::bytes rs;
            rs.reserve(bs.size() + 1);
            rs.push_back(p);
            rs.insert(rs.end(), bs.begin(), bs.end());
            return rs;
        }

        u::bytes session_library::encrypt_assymetric(session_map::const_iterator s, const u::bytes& bs) const
        {
            if(bs.empty()) return {};

            if(s == _s.end()) return {};

            auto es = s->second.key.encrypt(bs);
            return append_prefix(encryption_type::assymetric, es);
        }

        u::bytes session_library::encrypt_assymetric(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            return encrypt_assymetric(_s.find(i), bs);
        }


        u::bytes session_library::encrypt_plaintext(const u::bytes& bs) const
        {
            return append_prefix(encryption_type::plaintext, bs);
        }

        u::bytes session_library::encrypt_symmetric(session_map::const_iterator s, const u::bytes& bs) const
        {
            if(s == _s.end()) return {};
            REQUIRE(s->second.shared_secret.ready());

            auto es = s->second.shared_secret.encrypt(bs);
            return append_prefix(encryption_type::symmetric, es);
        }

        u::bytes session_library::encrypt_symmetric(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            return encrypt_symmetric(_s.find(i), bs);
        }

        u::bytes session_library::encrypt(const id& i, const u::bytes& bs) const
        {
            u::mutex_scoped_lock l(_mutex);
            auto s = _s.find(i);
            if(s == _s.end()) return encrypt_plaintext(bs); 

            if(!s->second.shared_secret.ready())
            {
                return encrypt_assymetric(s, bs);
            }

            return encrypt_symmetric(s, bs);
        }

        u::bytes session_library::decrypt(const id& i, const u::bytes& bs) const
        {
            if(bs.size() < 2) return {};

            u::bytes ds;
            auto message_start = bs.begin();
            message_start++;

            switch(bs[0])
            {
                case encryption_type::plaintext: 
                    {
                        ds.reserve(bs.size()-1);; 
                        auto b = bs.begin(); b++;
                        ds.insert(ds.begin(), b, bs.end());
                    }
                    break;
                case encryption_type::symmetric: 
                    {
                        u::mutex_scoped_lock l(_mutex);
                        auto s = _s.find(i);
                        if(s == _s.end()) return {};
                        if(!s->second.shared_secret.ready()) return {};
                        u::bytes cb{message_start, bs.end()};
                        ds = s->second.shared_secret.decrypt(cb);
                    }
                    break;
                case encryption_type::assymetric: 
                    {
                        //decrypt message, skipping encryption type prefix
                        u::bytes cb{message_start, bs.end()};
                        ds = _pk.decrypt(cb);
                    }
                    break;
                default: return {};
            }

            ENSURE(!ds.empty());
            return ds;
        }

        void session_library::create_session(const id& i, const public_key& key)
        {
            REQUIRE(key.valid());
            u::mutex_scoped_lock l(_mutex);

            LOG << "creating pk security session for: " << i << std::endl;

            auto& s = _s[i];
            s.key = key;

            ENSURE(!s.shared_secret.ready());
            ENSURE(s.key.valid());
        }

        void session_library::create_session(const id& i, const public_key& key, const util::bytes& public_val)
        {
            REQUIRE(key.valid());
            u::mutex_scoped_lock l(_mutex);

            LOG << "creating pk/dh security session for: " << i << std::endl;

            auto& s = _s[i];
            s.key = key;
            s.shared_secret.create_symmetric_key(public_val);

            ENSURE(s.shared_secret.ready());
        }

        const session& session_library::get_session(const id& i) const
        {
            u::mutex_scoped_lock l(_mutex);
            auto r = _s.find(i);
            REQUIRE(r != _s.end());

            return r->second;
        }

        void session_library::remove_session(const id& i)
        {
            u::mutex_scoped_lock l(_mutex);
            REQUIRE(_s.count(i));

            _s.erase(i);
        }
    }
}
