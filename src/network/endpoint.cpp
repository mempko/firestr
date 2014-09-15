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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#include "network/endpoint.hpp"
#include "util/string.hpp"

#include <boost/lexical_cast.hpp>

namespace u = fire::util;

namespace fire
{
    namespace network
    {
        using strings = std::vector<std::string>;

        std::string port_to_string(port_type p)
        {
            return boost::lexical_cast<std::string>(p);
        }

        port_type parse_port(const std::string& p)
        try
        {
            return boost::lexical_cast<port_type>(p);
        }
        catch(...)
        {
            throw std::invalid_argument("unable to parse port, port must be a number");
        }


        host_port parse_host_port(const std::string& a)
        {
            auto s = u::split<strings>(a, ":");
            if(s.size() != 2) std::invalid_argument("address must have host and port."); 
            return std::make_pair(s[0], parse_port(s[1]));
        }

    }
}
