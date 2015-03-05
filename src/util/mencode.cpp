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

#include "util/mencode.hpp"

#include <algorithm>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

namespace fire
{
    namespace util
    {
        namespace 
        {
            const std::string INT_TYPE = "int";
            const std::string SIZE_TYPE = "size";
            const std::string REAL_TYPE = "real";
        }

        using boost::lexical_cast;

        value::value() : _v{} {}
        value::value(bool v) : _v{v} {}
        value::value(int v) : _v{static_cast<int64_t>(v)} {}
        value::value(int64_t v) : _v{v} {}
        value::value(size_t v) : _v{v} {}
        value::value(double v) : _v{v} {}
        value::value(const std::string& v) : _v{to_bytes(v)} {}
        value::value(const bytes& v) : _v{v} {}
        value::value(const dict& v) : _v{v} {}
        value::value(const array& v) : _v{v} {}
        value::value(const value& o) : _v{o._v} {}

        value::operator bool() const { return as_bool();}
        value::operator int() const { return static_cast<int>(as_int());}
        value::operator int64_t() const { return as_int();}
        value::operator size_t() const { return as_size();}
        value::operator double() const { return as_double();}
        value::operator std::string() const { return as_string();}
        value::operator bytes() const { return as_bytes();}
        value::operator dict() const { return as_dict();}
        value::operator array() const { return as_array();}

        value& value::operator=(bool v) { _v = v; return *this;}
        value& value::operator=(int v) { _v = static_cast<int64_t>(v); return *this;}
        value& value::operator=(int64_t v) { _v = v; return *this;}
        value& value::operator=(size_t v) { _v = v; return *this;}
        value& value::operator=(double v) { _v = v; return *this;}
        value& value::operator=(const std::string& v) { _v = to_bytes(v); return *this;}
        value& value::operator=(const bytes& v) { _v = v; return *this;}
        value& value::operator=(const dict& v) { _v = v; return *this;}
        value& value::operator=(const array& v) { _v = v; return *this;}
        value& value::operator=(const value& o) 
        { 
            if(&o == this) return *this;
            _v = o._v; 
            return *this;
        }

        bool value::as_bool() const 
        try
        { 
            return boost::any_cast<bool>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an boolean");
        }

        int64_t value::as_int() const 
        try
        { 
            return boost::any_cast<int64_t>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an integer");
        }

        size_t value::as_size() const 
        try
        { 
            return boost::any_cast<size_t>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an size type");
        }

        double value::as_double() const 
        try
        { 
            return boost::any_cast<double>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an real");
        }

        std::string value::as_string() const
        try
        { 
            return to_str(boost::any_cast<const bytes&>(_v)); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not a string");
        }

        const bytes& value::as_bytes() const 
        try
        { 
            return boost::any_cast<const bytes&>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not a byte array");
        }

        const dict& value::as_dict() const 
        try
        { 
            return boost::any_cast<const dict&>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an dictionary");
        }

        const array& value::as_array() const 
        try
        { 
            return boost::any_cast<const array&>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an array");
        }

        dict& value::as_dict() 
        try
        { 
            return boost::any_cast<dict&>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an dictionary");
        }
        
        array& value::as_array() 
        try
        { 
            return boost::any_cast<array&>(_v); 
        }
        catch (...)
        {
            throw std::runtime_error("value is not an array");
        }
                
        bool value::is_bool() const { return _v.type() == typeid(bool);}
        bool value::is_int() const { return _v.type() == typeid(int64_t);}
        bool value::is_size() const { return _v.type() == typeid(size_t);}
        bool value::is_double() const { return _v.type() == typeid(double);}
        bool value::is_bytes() const { return _v.type() == typeid(bytes);}
        bool value::is_dict() const { return _v.type() == typeid(dict);}
        bool value::is_array() const { return _v.type() == typeid(array);}
        bool value::empty() const { return _v.empty();}

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
            if(p == _m.end()) 
            {
                std::stringstream e;
                e << "unable to find key `" << k << "' in dictionary" << std::endl;
                throw std::runtime_error{e.str()}; 
            }

            ENSURE(p != _m.end());
            return p->second;
        }

        size_t dict::size() const { return _m.size(); }

        bool dict::has(const std::string& k) const
        {
            return _m.count(k) > 0;
        }

        bool dict::remove(const std::string& k)
        {
            return _m.erase(k) > 0;
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
        void array::resize(size_t size) { _a.resize(size); }

        template <typename t>
        void enc(std::ostream& o, char s, char e, const t& v)
        {
            o << s << lexical_cast<std::string>(v) << e;
        }

        void encode(std::ostream& o, bool v) { o << ( v ? 'T' : 'F'); }
        void encode(std::ostream& o, int64_t v) { enc(o, 'i', ';', v); }
        void encode(std::ostream& o, size_t v) { enc(o, 's', ';', v); }
        void encode(std::ostream& o, double v) { enc(o, 'r', ';', v); }

        template<typename b>
        void encode_b(std::ostream& o, const b& v)
        {
            o << lexical_cast<std::string>(v.size()) << ':';
            o.write(&v[0], v.size()); 
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

        void encode_empty(std::ostream& o)
        {
            o << 'n';
        }

        void encode(std::ostream& o, const value& v)
        {
            if(v.empty()) encode_empty(o);
            else if(v.is_bool()) encode(o, v.as_bool());
            else if(v.is_int()) encode(o, v.as_int());
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
            e << "unexpected end of stream at byte " << i.tellg();
            throw std::runtime_error{e.str()}; 
        }

        struct work_space
        {
            std::string s;
            bytes b;
        };

        void get_until(std::istream& i, char e, work_space& w)
        {
            w.s.clear();

            int c = i.get();
            while(c != e && i.good())
            {
                w.s.push_back(c);
                c = i.get();
            }

            check(i);
            CHECK_EQUAL(c, e);
        }

        template<typename t>
            t dec(std::istream& i, const std::string& type, char b, char e, work_space& w)
            {
                int c = i.get();
                if(!i.good()) return t{};

                if(c != b)
                {
                    std::stringstream e;
                    e << "expected " << type << " at byte " << i.tellg();
                    throw std::runtime_error{e.str()}; 
                }

                get_until(i, e, w);
                return lexical_cast<t>(w.s);
            }

        bool decode_bool(std::istream& i) 
        { 
            bool r = false;
            int c = i.get();
            if(!i.good()) return r;

            if(c == 'T') r = true;
            else if(c == 'F') { /*do nothing*/}
            else
            {
                std::stringstream e;
                e << "expected boolean at byte " << i.tellg();
                throw std::runtime_error{e.str()}; 
            }
            return r;
        }

        int64_t decode_int(std::istream& i, work_space& w) 
        { 
            return dec<int64_t>(i, INT_TYPE, 'i', ';', w); 
        }

        size_t decode_size(std::istream& i, work_space& w)
        {
            return dec<size_t>(i, SIZE_TYPE, 's', ';', w);
        }

        double decode_double(std::istream& i, work_space& w)
        {
            return dec<double>(i, REAL_TYPE, 'r', ';', w);
        }

        void decode_empty(std::istream& i)
        {
            int c = i.get();
            if(!i.good()) return;

            if(c != 'n')
            {
                std::stringstream e;
                e << "expected empty at byte " << i.tellg();
                throw std::runtime_error{e.str()}; 
            }

            return;
        }

        void decode_bytes(std::istream& i, work_space& w)
        {
            w.b.clear();

            int c = i.peek();
            if(!i.good()) return;

            if(c < '0' || c > '9')
            {
                std::stringstream e;
                e << "expected byte string at byte " << i.tellg() << " but got `" << char(c) << "'";
                throw std::runtime_error{e.str()}; 
            }

            get_until(i, ':', w);
            size_t size = lexical_cast<size_t>(w.s);
            if(size == 0) return;

            w.b.resize(size);
            i.read(&w.b[0], size);
        }

        void decode_key(std::istream& i, work_space& w)
        {
            int c = i.peek();
            if(!i.good()) return;

            if(c < '0' || c > '9')
            {
                std::stringstream e;
                e << "expected byte key at byte " << i.tellg() << " but got `" << char(c) << "'";
                throw std::runtime_error{e.str()}; 
            }

            get_until(i, ':', w);
            size_t size = lexical_cast<size_t>(w.s);
            if(size == 0) return;

            w.s.clear();
            w.s.resize(size);
            i.read(&w.s[0], size);
        }

        dict decode_dict(std::istream& i, work_space& w);
        array decode_array(std::istream& i, work_space& w);
        value decode_value(std::istream& i, work_space& w)
        {
            value v;

            int c = i.peek();
            if(!i.good()) return v;

            if(c == 'F' || c == 'T') v = decode_bool(i);
            else if(c == 'i') v = decode_int(i, w);
            else if(c == 's') v = decode_size(i, w);
            else if(c == 'r') v = decode_double(i, w);
            else if(c == 'd') v = decode_dict(i, w);
            else if(c == 'a') v = decode_array(i, w);
            else if(c == 'n') decode_empty(i);
            else if(c >= '0' && c <= '9') 
            {
                decode_bytes(i, w);
                v = w.b;
            }
            else 
            {
                std::stringstream e;
                e << "unexpected value type `" << c << "' at byte " << i.tellg();
                throw std::runtime_error{e.str()};
            }
            return v;
        }

        dict decode_dict(std::istream& i, work_space& w)
        {
            dict d;
            int c = i.get();
            if(!i.good()) return d;

            if(c != 'd') 
            {
                std::stringstream e;
                e << "expected dictionary at byte " << i.tellg();
                throw std::runtime_error{e.str()};
            }

            c = i.peek();

            std::string k;
            while(c != ';' && i.good())
            {
                decode_key(i, w);
                k = w.s;
                d[k] = decode_value(i, w);

                c = i.peek();
            }
            c = i.get();
            check(i);
            CHECK_EQUAL(c, ';');

            return d;
        }

        array decode_array(std::istream& i, work_space& w)
        {
            array a;
            int c = i.get();
            if(!i.good()) return a;

            if(c != 'a') 
            {
                std::stringstream e;
                e << "expected array at byte " << i.tellg();
                throw std::runtime_error{e.str()};
            }

            c = i.peek();

            while(c != ';' && i.good())
            {
                a.add(decode_value(i, w));
                c = i.peek();
            }

            c = i.get();
            check(i);
            CHECK_EQUAL(c, ';');

            return a;
        }

        std::istream& operator>>(std::istream& i, dict& v)
        {
            work_space w;
            v = decode_dict(i, w);
            return i;
        }

        std::istream& operator>>(std::istream& i, array& v)
        {
            work_space w;
            v = decode_array(i, w);
            return i;
        }
        
        std::istream& operator>>(std::istream& i, value& v)
        {
            work_space w;
            v = decode_value(i, w);
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
        if(!i.good()) {v.clear(); return i;}
        fire::util::work_space w;
        auto vv = fire::util::decode_value(i, w);
        if(vv.is_bytes()) v = vv.as_bytes();
        else v.clear();
        return i;
    }
}
