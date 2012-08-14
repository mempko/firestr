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

#include "util/mencode.hpp"

#include <boost/lexical_cast.hpp>

namespace fire
{
    namespace util
    {
        using boost::lexical_cast;

        value_holder dict::operator[](const std::string& k)
        {
            return value_holder(_m[k]);
        }

        size_t dict::size() const { return _m.size(); }

        bool dict::has(const std::string& k) const
        {
            return _m.count(k) > 0;
        }

        dict::const_iterator dict::begin() const { return _m.begin(); }
        dict::const_iterator dict::end() const { return _m.end(); }

        value_holder array::operator[](size_t i)
        {
            return value_holder(_a[i]);
        }

        size_t array::size() const { return _a.size(); }

        array::const_iterator array::begin() const { return _a.begin(); }
        array::const_iterator array::end() const { return _a.end(); }

        void array::add(int v) { _a.push_back(v); }
        void array::add(size_t v) { _a.push_back(v); }
        void array::add(double v) { _a.push_back(v); }
        void array::add(const std::string& v) { _a.push_back(to_bytes(v)); }
        void array::add(const bytes& v) { _a.push_back(v); }
        void array::add(const dict& v) { _a.push_back(v); }
        void array::add(const array& v) { _a.push_back(v); }

        bool is_int(const value& v) { return v.type() == typeid(int);}
        bool is_size(const value& v) { return v.type() == typeid(size_t);}
        bool is_double(const value& v) { return v.type() == typeid(double);}
        bool is_bytes(const value& v) { return v.type() == typeid(bytes);}
        bool is_dict(const value& v) { return v.type() == typeid(dict);}
        bool is_array(const value& v) { return v.type() == typeid(array);}


        template <typename t>
        void enc(std::ostream& o, char s, char e, const t& v)
        {
            o << s << lexical_cast<std::string>(v) << e;
        }

        void encode(std::ostream& o, int v) { enc(o, 'i', ';', v); }
        void encode(std::ostream& o, size_t v) { enc(o, 's', ';', v); }
        void encode(std::ostream& o, double v) { enc(o, 'r', ';', v); }

        template<typename b>
        void encode_b(std::ostream& o, const b& v)
        {
            o << lexical_cast<std::string>(v.size()) << ':';
            for(auto c : v) o << c;
        }

        void encode(std::ostream& o, const bytes& v) { encode_b(o, v); }
        void encode(std::ostream& o, const std::string& v) { encode_b(o, v); }

        void encode(std::ostream& o, const value& v);
        void encode(std::ostream& o, const dict& v)
        {
            o << 'd';
            for(const auto& p : v)
            {
                encode(o, p.first);
                encode(o, p.second);
            }
            o << ';';
        }

        void encode(std::ostream& o, const array& v)
        {
            o << 'd';
            encode(o, v.size());
            for(const auto& e : v) encode(o, e);
            o << ';';
        }

        void encode(std::ostream& o, const value_holder& v)
        {
            if(v.is_int()) encode(o, v.as_int());
            else if(v.is_size()) encode(o, v.as_size());
            else if(v.is_double()) encode(o, v.as_double());
            else if(v.is_bytes()) encode(o, v.as_bytes());
            else if(v.is_dict()) encode(o, v.as_dict());
            else if(v.is_array()) encode(o, v.as_array());
            else CHECK(false && "missed case");
        }

        void encode(std::ostream& o, const value& v)
        {
            encode(o, value_holder(v));
        }

        std::ostream& operator<<(std::ostream& o, const dict& v)
        {
            encode(o, v);
            return o;
        }

        std::ostream& operator<<(std::ostream& o, const array& v)
        {
            encode(o, v);
            return o;
        }

        std::ostream& operator<<(std::ostream& o, const value_holder& v)
        {
            encode(o, v);
            return o;
        }
    }
}
