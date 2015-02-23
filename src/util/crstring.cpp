/*
 * Copyright (C) 2014  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vedit_refsion 3 of the License, or
 * (at your option) any later vedit_refsion.
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

#include "util/crstring.hpp"
#include "util/text.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace fire 
{
    namespace util 
    {
        cr_string::cr_string() : _c{""} {}
        cr_string::cr_string(const std::string& id) : _c{id} {}
        cr_string::cr_string(const tracked_sclock& c, const std::string& s) : _c{c}, _s{s} {}

        const std::string& cr_string::str() const { return _s; }
        const tracked_sclock& cr_string::clock() const { return _c; }
        tracked_sclock& cr_string::clock() { return _c; }

        void cr_string::init_set(const std::string& s) { _s = s;}
        void cr_string::set(const std::string& s) { _s = s; _c++;}

        merge_result cr_string::merge(const cr_string& o)
        {
            auto cmp = o._c.compare(_c);

            merge_result r = merge_result::NO_CHANGE;

            //merge strings
            switch(cmp)
            {
                case -1: /*do nothing */ break;

                //string is newer then set current to new
                case 1: _s = o._s; r = merge_result::UPDATED; break;

                //string is concurrent, do 3 way merge
                case 0: 
                    {
                        if(_c.identical(o._c)) break;

                        //check to see if we have last seen string from other node
                        //and use it as base string, otherwise use other nodes string
                        auto a = o._s;
                        auto b = _s;
                        auto c = o._s;

                        auto merged = util::merge(a, b, c, _s);
                        if(!merged) r = merge_result::CONFLICT;
                        else
                        {
                            r = merge_result::MERGED;
                            _c++;
                        }
                    }
                    break;
                default:
                    CHECK(false && "missed case");
            }

            //merge clocks
            _c += o._c;

            return r;
        }

    }
}
