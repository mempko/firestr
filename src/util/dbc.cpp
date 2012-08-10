
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
#include "util/dbc.hpp"

namespace fire 
{
    namespace util 
    {
        void raise(const char * msg) 
        {
            std::cerr << msg << std::endl;
            exit(1);
        }

        void raise1( const char * file, const int line, const char * dbc, const char * expr)
        {
            std::stringstream s;
            s << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            s << "!! " << dbc << " failed" << std::endl;
            s << "!! expr: " << expr << std::endl;
            s << "!! file: " << file << " (" << line << ")" << std::endl;
            s << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            raise(s.str().c_str());
        }
    }
}
