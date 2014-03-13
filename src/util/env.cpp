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

#include "util/env.hpp"

#include <cstdlib>
#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

namespace fire
{
    namespace util
    {
        std::string get_home_dir()
        {
#ifdef _WIN64
            const char* home = std::getenv("USERPROFILE");
#else
            const char* home = std::getenv("HOME");
#endif
            return home != nullptr ? home : ".";
        }

        std::string get_default_firestr_home()
        {
#ifdef _WIN64
            //root in /Users/<user>/Application Data/
            const char* app_data = std::getenv("APPDATA");
            bf::path root = app_data != nullptr ? app_data : "./";
#elif __APPLE__
            //~/Library/Application Support/
            const char* home = std::getenv("HOME");
            bf::path root = home != nullptr ? home : "./";
            root /= "Library"; 
            root /= "Application Support";
#else
            //~/.config/firestr
            const char* home = std::getenv("HOME");
            bf::path root = home != nullptr ? home : "./";
            root /= ".config";
#endif

            bf::path r = root / "firestr";
            return r.string();
        }
    }
}
