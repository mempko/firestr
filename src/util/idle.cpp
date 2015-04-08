/*
 * Copyright (C) 2015  Maxim Noah Khailo
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
#include "util/idle.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#ifdef _WIN64
#elif __APPLE__
#else
#include <X11/extensions/scrnsaver.h>
#endif

namespace fire
{
    namespace util
    {
        namespace
        {
            //const size_t IDLE_TIME = 5 * 60 * 1000; 
            const size_t IDLE_TIME = 10 * 1000; //10 seconds 
        }

        bool user_is_idle()
        {
            return user_idle() >= IDLE_TIME;
        }

#ifdef _WIN64
        size_t user_idle()
        {
            return 0.0;
        }
#elif __APPLE__
        size_t user_idle()
        {
            return 0.0;
        }
#else
        size_t user_idle()
        {
            static Display* d = nullptr;
            static XScreenSaverInfo* n = nullptr;
            if(!d) d = XOpenDisplay(0);
            if(!n) n = XScreenSaverAllocInfo();

            if(!d) 
            {
                LOG << "unable to open the x display" << std::endl; 
                return 0.0;
            }

            CHECK(n)
            XScreenSaverQueryInfo(d, DefaultRootWindow(d), n);

            return n->idle;
        }
#endif
    }
}
