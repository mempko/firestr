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
                value_holder(boost::any& v) : _v(v) {}

            public:
                operator int() { return as_int();}
                operator size_t() { return as_size();}
                operator double() { return as_double();}
                operator std::string() { return as_string();}
                operator bytes() { return as_bytes();}
                operator dict() { return as_dict();}
                operator array() { return as_array();}

            public:
                void operator=(int v) { _v = v;}
                void operator=(size_t v) { _v = v;}
                void operator=(double v) { _v = v;}
                void operator=(const std::string& v) { _v = to_bytes(v);}
                void operator=(const bytes& v) { _v = v;}
                void operator=(const dict& v) { _v = v;}
                void operator=(const array& v) { _v = v;}
            public:

                int as_int() const { return boost::any_cast<int>(_v); }
                size_t as_size() const { return boost::any_cast<size_t>(_v); }
                double as_double() const { return boost::any_cast<double>(_v); }
                std::string as_string() const { return to_str(boost::any_cast<const bytes&>(_v)); }
                const bytes& as_bytes() const { return boost::any_cast<const bytes&>(_v); }
                const dict& as_dict() const { return boost::any_cast<const dict&>(_v); }
                const array& as_array() const { return boost::any_cast<const array&>(_v); }
            private:
                value& _v;
        };
    }
}


#endif
