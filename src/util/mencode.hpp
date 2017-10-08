/*
 * Copyright (C) 2017  Maxim Noah Khailo
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

#pragma once

#include <fstream>
#include <initializer_list>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <memory>

#include <boost/any.hpp>

#include "util/bytes.hpp"
#include "util/dbc.hpp"

namespace fire::util
{
    class dict;
    class array;

    class value
    {
        public:
            value();
            value(bool v);
            value(int v);
            value(int64_t v);
            value(size_t v);
            value(double v);
            value(const std::string& v);
            value(const bytes& v);
            value(const dict& v);
            value(const array& v);
            value(const value& o);

        public:
            operator bool() const;
            operator int() const;
            operator int64_t() const;
            operator size_t() const;
            operator double() const;
            operator std::string() const;
            operator bytes() const;
            operator dict() const;
            operator array() const;

        public:
            value& operator=(bool v);
            value& operator=(int v);
            value& operator=(int64_t v);
            value& operator=(size_t v);
            value& operator=(double v);
            value& operator=(const std::string& v);
            value& operator=(const bytes& v);
            value& operator=(const dict& v);
            value& operator=(const array& v);
            value& operator=(const value& o);

        public:
            bool as_bool() const;
            int64_t as_int() const;
            size_t as_size() const;
            double as_double() const;
            std::string as_string() const;
            const bytes& as_bytes() const;
            const dict& as_dict() const;
            const array& as_array() const;
            dict& as_dict();
            array& as_array();

        public:
            bool is_bool() const;
            bool is_int() const;
            bool is_size() const;
            bool is_double() const;
            bool is_bytes() const;
            bool is_dict() const;
            bool is_array() const;
            bool empty() const;

        private:
            boost::any _v;
    };

    using kv = std::pair<std::string, value>;

    class dict
    {
        private:
            using value_map = std::unordered_map<std::string, value>;

        public:
            using value_type = value_map::value_type;

        public:
            dict();
            dict(std::initializer_list<kv>);

        public:
            using const_iterator = value_map::const_iterator;
            using iterator = value_map::iterator;

        public:
            value& operator[](const std::string& k);
            const value& operator[] (const std::string& k) const;

            size_t size() const;
            bool has(const std::string& k) const; 
            bool remove(const std::string& k);

        public:
            const_iterator begin() const;
            const_iterator end() const;
            iterator begin();
            iterator end();

        private:
            value_map _m;
    };

    using dict_ptr = std::shared_ptr<dict>;

    class array
    {
        private:
            using value_array = std::vector<value>;

        public:
            using const_iterator = value_array::const_iterator;
            using iterator = value_array::iterator;

        public:
            array();
            array(std::initializer_list<value>);

        public:
            void add(const value&);
            void resize(size_t size);

        public:
            value& operator[](size_t);
            const value& operator[] (size_t) const;
            size_t size() const;

        public:
            const_iterator begin() const;
            const_iterator end() const;
            iterator begin();
            iterator end();

        private:
            value_array _a;
    };

    template<class C>
        array to_array(const C& cs)
        {
            array a;
            for(const auto& c : cs) a.add(c);

            ENSURE_EQUAL(a.size(), cs.size());
            return a;
        }

    template<class C, class Convert>
        array to_array(const C& cs, Convert conv)
        {
            array a;
            for(const auto& c : cs) a.add(conv(c));

            ENSURE_EQUAL(a.size(), cs.size());
            return a;
        }

    template<class C, class Convert>
        void from_array(const array& a, C& cs, Convert conv)
        {
            for(const auto& v : a)
                cs.emplace_back(conv(v));

            ENSURE_EQUAL(cs.size(), a.size());
        }

    std::ostream& operator<<(std::ostream&, const dict&);
    std::ostream& operator<<(std::ostream&, const array&);
    std::ostream& operator<<(std::ostream&, const value&);

    std::istream& operator>>(std::istream&, dict&);
    std::istream& operator>>(std::istream&, array&);
    std::istream& operator>>(std::istream&, value&);

    template <typename type> 
        bytes encode(const type& v)
        {
            std::stringstream s;
            s << v;
            return to_bytes(s.str());
        }

    template <typename type> 
        void decode(const bytes& b, type& v)
        {
            std::stringstream s{to_str(b)};
            s >> v;
        }

    template <typename type> 
        type decode(const bytes& b)
        {
            type v;
            std::stringstream s{to_str(b)};
            s >> v;
            return v;
        }

    template<class R>
        bool load_from_file(const std::string& f, R& r)
        {
            std::ifstream in(f.c_str(), std::fstream::in | std::fstream::binary);
            if(!in.good()) return false;
            in >> r;
            return true;
        }

    template<class D>
        void save_to_file(const std::string& f, const D& d)
        {
            std::ofstream out(f.c_str(), std::fstream::out | std::fstream::binary);
            if(!out.good()) 
                throw std::runtime_error{"unable to save `" + f + "'"};

            out << d;
        }
}

namespace std
{
    std::ostream& operator<<(std::ostream&, const fire::util::bytes&);
    std::istream& operator>>(std::istream&, fire::util::bytes&);
}

