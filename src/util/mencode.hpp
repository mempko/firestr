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

#ifndef FIRESTR_UIL_MENCODE_H
#define FIRESTR_UIL_MENCODE_H

#include <map>
#include <iostream>

#include <boost/any.hpp>

#include "util/bytes.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace util
    {
        typedef boost::any value;

        class value_holder;

        class dict
        {
            private:
                typedef std::map<std::string, value> value_map;

            public:
                typedef value_map::const_iterator const_iterator;
                
            public:
                value_holder operator[](const std::string& k);
                size_t size() const;
                bool has(const std::string& k) const; 

            public:
                value_map::const_iterator begin() const;
                value_map::const_iterator end() const;

            private:
                value_map _m;
        };

        class array
        {
            private:
                typedef std::vector<value> value_array;

            public:
                typedef value_array::const_iterator const_iterator;

            public:
                void add(int);
                void add(size_t);
                void add(double);
                void add(const std::string&);
                void add(const bytes&);
                void add(const dict&);
                void add(const array&);
                void add(const value_holder&);

            public:
                value_holder operator[](size_t);
                size_t size() const;

            public:
                const_iterator begin() const;
                const_iterator end() const;

            private:
                value_array _a;
        };


        class value_holder
        {
            public:
                value_holder(boost::any& v) : 
                    _v(&v), _const(false) 
                {
                    INVARIANT(_v);
                }

                value_holder(const boost::any& v) : 
                    _v(const_cast<boost::any*>(&v)), _const(true)
                {
                    INVARIANT(_v);
                }

                value_holder(const value_holder& o) : 
                    _v(const_cast<boost::any*>(o._v)), _const(o._const)
                {
                    INVARIANT(_v);
                }

            public:
                operator int() { return as_int();}
                operator size_t() { return as_size();}
                operator double() { return as_double();}
                operator std::string() { return as_string();}
                operator bytes() { return as_bytes();}
                operator dict() { return as_dict();}
                operator array() { return as_array();}

            public:
                value_holder& operator=(int v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(size_t v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(double v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(const std::string& v) { CHECK_FALSE(_const); *_v = to_bytes(v); return *this;}
                value_holder& operator=(const bytes& v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(const dict& v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(const array& v) { CHECK_FALSE(_const); *_v = v; return *this;}
                value_holder& operator=(const value_holder& v) 
                { 
                    REQUIRE(v._v);
                    CHECK_FALSE(_const); 
                    *_v = *v._v; 
                    return *this;
                }

            public:
                int as_int() const { return boost::any_cast<int>(*_v); }
                size_t as_size() const { return boost::any_cast<size_t>(*_v); }
                double as_double() const { return boost::any_cast<double>(*_v); }
                std::string as_string() const { return to_str(boost::any_cast<const bytes&>(*_v)); }
                const bytes& as_bytes() const { return boost::any_cast<const bytes&>(*_v); }
                const dict& as_dict() const { return boost::any_cast<const dict&>(*_v); }
                const array& as_array() const { return boost::any_cast<const array&>(*_v); }
                const value& as_value() const { return *_v; }
                
            public:
                bool is_int() const { return _v->type() == typeid(int);}
                bool is_size() const { return _v->type() == typeid(size_t);}
                bool is_double() const { return _v->type() == typeid(double);}
                bool is_bytes() const { return _v->type() == typeid(bytes);}
                bool is_dict() const { return _v->type() == typeid(dict);}
                bool is_array() const { return _v->type() == typeid(array);}

            private:
                value* _v;
                bool _const;
        };

        std::ostream& operator<<(std::ostream&, const dict&);
        std::ostream& operator<<(std::ostream&, const array&);
        std::ostream& operator<<(std::ostream&, const value_holder&);

        std::istream& operator>>(std::istream&, dict&);
        std::istream& operator>>(std::istream&, array&);
        std::istream& operator>>(std::istream&, value_holder&);
    }
}

namespace std
{
    std::ostream& operator<<(std::ostream&, const fire::util::bytes&);
    std::istream& operator>>(std::istream&, fire::util::bytes&);
}

#endif
