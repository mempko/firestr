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

namespace fire
{
    namespace util
    {
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
    }
}
