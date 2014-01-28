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
#include "util/log.hpp"

#include "util/dbc.hpp"
#include "util/filesystem.hpp"

#include <exception>
#include <fstream>

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

namespace fire 
{
    namespace util 
    {
        namespace
        {
            std::string LOG_HOME = "log";
            std::string LOG_FILE = "out.log";
        }

        log_ptr log::_log;

        std::string get_log_path(bf::path home)
        {
            bf::path log_home = home / LOG_HOME;
            return log_home.string();
        }

        std::string get_log_file(bf::path path)
        {
            bf::path log_file = path / LOG_FILE;
            return log_file.string();
        }

        log::log(std::string home)
        {
            auto log_path = get_log_path(home);
            if(!create_directory(log_path))
                throw std::runtime_error{"unable to create log directory `" + log_path + "'"};

            _log_file = get_log_file(log_path);
            ostream_ptr s{new std::ofstream(_log_file.c_str())};
            if(!s->good())
                throw std::runtime_error{"unable to create log file `" + _log_file + "'"};

            _stream = std::move(s);
        }

        std::ostream& log::stream()
        {
            INVARIANT(_stream);
            return *_stream;
        }

        const std::string& log::path()
        {
            return _log_file;
        }

        void log::create_log(const std::string& home)
        {
            REQUIRE_FALSE(_log);
            _log.reset(new log(home));
        }

        log* log::inst()
        {
            return _log.get();
        }
    }
}
