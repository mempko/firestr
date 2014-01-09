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

#ifndef FIRESTR_UTIL_STRING_H
#define FIRESTR_UTIL_STRING_H

#include <string>
#include <vector>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "util/dbc.hpp"

namespace fire
{
    namespace util
    {
        template <class container>
            container split(
                    const std::string& s, 
                    const std::string& delimiters)
            {
                container result;
                boost::char_separator<char> d{delimiters.c_str()};
                boost::tokenizer<boost::char_separator<char>> tokens{s, d};
                for(auto t : tokens)
                {
                    boost::algorithm::trim(t);
                    if(!t.empty()) result.insert(result.end(), t);
                }
                ENSURE_FALSE(result.empty());
                return result;
            }

        using string_vect = std::vector<std::string>;
        using string_set = std::set<std::string>;
    }

}

#endif
