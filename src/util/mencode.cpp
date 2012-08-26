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

#include <algorithm>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

namespace fire
{
    namespace util
    {
        using boost::lexical_cast;

        value::value() : _v{} {}
        value::value(int v) : _v{v} {}
        value::value(size_t v) : _v{v} {}
        value::value(double v) : _v{v} {}
        value::value(const std::string& v) : _v{to_bytes(v)} {}
        value::value(const char* v) : _v{to_bytes(v)} {}
        value::value(const bytes& v) : _v{v} {}
        value::value(const dict& v) : _v{v} {}
        value::value(const array& v) : _v{v} {}
        value::value(const value& o) : _v{o._v} {}

        value::operator int() { return as_int();}
        value::operator size_t() { return as_size();}
        value::operator double() { return as_double();}
        value::operator std::string() { return as_string();}
        value::operator bytes() { return as_bytes();}
        value::operator dict() { return as_dict();}
        value::operator array() { return as_array();}

        value& value::operator=(int v) { _v = v; return *this;}
        value& value::operator=(size_t v) { _v = v; return *this;}
        value& value::operator=(double v) { _v = v; return *this;}
        value& value::operator=(const std::string& v) { _v = to_bytes(v); return *this;}
        value& value::operator=(const char* v) { _v = to_bytes(v); return *this;}
        value& value::operator=(const bytes& v) { _v = v; return *this;}
        value& value::operator=(const dict& v) { _v = v; return *this;}
        value& value::operator=(const array& v) { _v = v; return *this;}
        value& value::operator=(const value& o) 
        { 
            if(&o == this) return *this;
            _v = o._v; 
            return *this;
        }

        int value::as_int() const { return boost::any_cast<int>(_v); }
        size_t value::as_size() const { return boost::any_cast<size_t>(_v); }
        double value::as_double() const { return boost::any_cast<double>(_v); }
        std::string value::as_string() const { return to_str(boost::any_cast<const bytes&>(_v)); }
        const bytes& value::as_bytes() const { return boost::any_cast<const bytes&>(_v); }
        const dict& value::as_dict() const { return boost::any_cast<const dict&>(_v); }
        const array& value::as_array() const { return boost::any_cast<const array&>(_v); }
                
        bool value::is_int() const { return _v.type() == typeid(int);}
        bool value::is_size() const { return _v.type() == typeid(size_t);}
        bool value::is_double() const { return _v.type() == typeid(double);}
        bool value::is_bytes() const { return _v.type() == typeid(bytes);}
        bool value::is_dict() const { return _v.type() == typeid(dict);}
        bool value::is_array() const { return _v.type() == typeid(array);}

        dict::dict() : _m{} {}
        dict::dict(std::initializer_list<kv> s)
        {
            for(auto v : s) _m[v.first] = v.second; 

            ENSURE_LESS_EQUAL(_m.size(), s.size());
        }

        value& dict::operator[](const std::string& k)
        {
            return _m[k];
        }

        const value& dict::operator[] (const std::string& k) const
        {
            auto p = _m.find(k);
            REQUIRE_FALSE(p == _m.end());
            return p->second;
        }

        size_t dict::size() const { return _m.size(); }

        bool dict::has(const std::string& k) const
        {
            return _m.count(k) > 0;
        }

        dict::const_iterator dict::begin() const { return _m.begin(); }
        dict::const_iterator dict::end() const { return _m.end(); }
        dict::iterator dict::begin() { return _m.begin(); }
        dict::iterator dict::end() { return _m.end(); }

        array::array() : _a{} {}
        array::array(std::initializer_list<value> s)
        {
            _a.resize(s.size());
            std::copy(s.begin(), s.end(), _a.begin());

            ENSURE_EQUAL(_a.size(), s.size());
        }

        value& array::operator[](size_t i)
        {
            return _a[i];
        }

        const value& array::operator[] (size_t i) const
        {
            return _a[i];
        }

        size_t array::size() const { return _a.size(); }

        array::const_iterator array::begin() const { return _a.begin(); }
        array::const_iterator array::end() const { return _a.end(); }
        array::iterator array::begin() { return _a.begin(); }
        array::iterator array::end() { return _a.end(); }

        void array::add(const value& v) { _a.push_back(v); }

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
            o << 'a';
            for(const auto& e : v) encode(o, e);
            o << ';';
        }

        void encode(std::ostream& o, const value& v)
        {
            if(v.is_int()) encode(o, v.as_int());
            else if(v.is_size()) encode(o, v.as_size());
            else if(v.is_double()) encode(o, v.as_double());
            else if(v.is_bytes()) encode(o, v.as_bytes());
            else if(v.is_dict()) encode(o, v.as_dict());
            else if(v.is_array()) encode(o, v.as_array());
            else CHECK(false && "missed case");
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

        std::ostream& operator<<(std::ostream& o, const value& v)
        {
            encode(o, v);
            return o;
        }

        void check(std::istream& i)
        {
            if(i.good()) return;
            std::stringstream e;
            e << "unexpected end of stream at byte " << e.tellg();
            throw std::runtime_error{e.str()}; 
        }

        std::string get_until(std::istream& i, char e)
        {
            std::string s;
            s.reserve(64);

            int c = i.get();
            while(c != e && i.good())
            {
                s.push_back(c);
                c = i.get();
            }

            check(i);
            CHECK_EQUAL(c, e);

            return s;
        }

        template<typename t>
            t dec(std::istream& i, const std::string& type, char b, char e)
            {
                int c = i.get();
                if(!i.good()) t{};

                if(c != b)
                {
                    std::stringstream e;
                    e << "expected " << type << " at byte " << e.tellg();
                    throw std::runtime_error{e.str()}; 
                }

                std::string sv = get_until(i, e);
                return lexical_cast<t>(sv);
            }

        int decode_int(std::istream& i) 
        { 
            return dec<int>(i, "int", 'i', ';'); 
        }

        size_t decode_size(std::istream& i)
        {
            return dec<size_t>(i, "size", 's', ';');
        }

        double decode_double(std::istream& i)
        {
            return dec<double>(i, "real", 'r', ';');
        }

        bytes decode_bytes(std::istream& i)
        {
            bytes b;
            int c = i.get();
            if(!i.good()) return b;

            if(c < '0' || c > '9')
            {
                std::stringstream e;
                e << "expected byte string at byte " << e.tellg() << " but got `" << char(c) << "'";
                throw std::runtime_error{e.str()}; 
            }

            std::string ss;
            ss.push_back(c);
            ss.append(get_until(i, ':'));
            size_t size = lexical_cast<size_t>(ss);
            if(size == 0) return b;

            b.resize(size);
            i.read(&b[0], size);

            return b;
        }

        dict decode_dict(std::istream& i);
        array decode_array(std::istream& i);
        value decode_value(std::istream& i)
        {
            value v;

            int c = i.peek();
            if(!i.good()) return v;

            if(c == 'i') v = decode_int(i);
            else if(c == 's') v = decode_size(i);
            else if(c == 'r') v = decode_double(i);
            else if(c == 'd') v = decode_dict(i);
            else if(c == 'a') v = decode_array(i);
            else if(c >= '0' && c <= '9') v = decode_bytes(i);
            else 
            {
                std::stringstream e;
                e << "unexpected value type `" << c << "' at byte " << i.tellg();
                throw std::runtime_error{e.str()};
            }
            return v;
        }

        dict decode_dict(std::istream& i)
        {
            dict d;
            int c = i.get();
            if(!i.good()) return d;

            if(c != 'd') 
            {
                std::stringstream e;
                e << "expected dictionary at byte " << e.tellg();
                throw std::runtime_error{e.str()};
            }

            c = i.peek();

            while(c != ';' && i.good())
            {
                std::string key = to_str(decode_bytes(i));
                d[key] = decode_value(i);
                c = i.peek();
            }
            c = i.get();
            check(i);
            CHECK_EQUAL(c, ';');

            return d;
        }

        array decode_array(std::istream& i)
        {
            array a;
            int c = i.get();
            if(!i.good()) return a;

            if(c != 'a') 
            {
                std::stringstream e;
                e << "expected array at byte " << e.tellg();
                throw std::runtime_error{e.str()};
            }

            c = i.peek();

            while(c != ';' && i.good())
            {
                a.add(decode_value(i));
                c = i.peek();
            }

            c = i.get();
            check(i);
            CHECK_EQUAL(c, ';');

            return a;
        }

        std::istream& operator>>(std::istream& i, dict& v)
        {
            v = decode_dict(i);
            return i;
        }

        std::istream& operator>>(std::istream& i, array& v)
        {
            v = decode_array(i);
            return i;
        }
        
        std::istream& operator>>(std::istream& i, value& v)
        {
            v = decode_value(i);
            return i;
        }
    }
}

namespace std
{
    std::ostream& operator<<(std::ostream& o, const fire::util::bytes& v)
    {
        fire::util::encode(o, v);
        return o;
    }

    std::istream& operator>>(std::istream& i, fire::util::bytes& v)
    {
        v = fire::util::decode_value(i).as_bytes();
        return i;
    }
}
