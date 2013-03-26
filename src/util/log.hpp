/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#ifndef FIRESTR_LOG_H
#define FIRESTR_LOG_H

#include <string>
#include <iostream>
#include <memory>

namespace fire 
{
    namespace util 
    {
        typedef std::unique_ptr<std::ostream> ostream_ptr;

        class log;
        typedef std::unique_ptr<log> log_ptr;

        class log
        {
            private:
                log(std::string home);

            public:
                std::ostream& stream();

            private:
                std::string _log_file;
                ostream_ptr _stream;

            public:
                static void create_log(const std::string& home);
                static log* inst();


            private:
                static log_ptr _log;
        };
    }
}

#define LOG if(fire::util::log::inst()) fire::util::log::inst()->stream() 
#define CREATE_LOG(PATH) fire::util::log::create_log((PATH))

#endif
