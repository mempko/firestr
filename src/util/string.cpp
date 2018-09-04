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

#include "util/string.hpp"

#include <QByteArray>

namespace fire::util
{
    void trim(std::string& t) 
    { 
        boost::algorithm::trim(t);
    }

    /**
     * Creates a base 64 string with width of 64 characters
     */
    std::string to_base_64(const std::string& s)
    {
        QByteArray b{s.c_str()};
        const std::string bs = b.toBase64().data();

        size_t stride = 64;
        size_t st = 0;
        std::stringstream r;
        while(st < bs.size())
        {
            r << bs.substr(st, std::min(stride, bs.size() - st)) << std::endl;
            st += stride;
        }

        return r.str();
    }

    bool good_char(char c)
    {
        return c != '\n' && c != '\r' && c !=' ' && c != '\t'; 
    }

    /**
     * Parses a base 64 string ignoring new line and space characters
     */
    std::string from_base_64(const std::string& s)
    {
        std::stringstream ss;
        for(auto c : s)
            if(good_char(c)) 
                ss << c;

        QByteArray bs{ss.str().c_str()};
        const auto ra = QByteArray::fromBase64(bs); 
        return ra.data();
    }
}
