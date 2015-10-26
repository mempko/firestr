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

#include "network/message_queue.hpp"

#include "util/string.hpp"

#include <vector>

namespace fire
{
    namespace network
    {
        using strings = std::vector<std::string>;

        queue_options parse_options(const strings& ss)
        {
            REQUIRE_FALSE(ss.empty());
            
            queue_options o;
            for(auto s : ss)
            {
                auto p = util::split<strings>(s, "=");
                if(p.size() == 1) o[p[0]] = "1";
                else if(p.size() == 2) o[p[0]] = p[1];
                else std::invalid_argument("The queue options `" + s + "' is invalid. Cannot have more than one `='"); 
            }

            ENSURE_EQUAL(ss.size(), o.size());
            return o;
        }

        address_components parse_address(
                const std::string& queue_address, 
                const queue_options& defaults)
        {
            auto s = util::split<strings>(queue_address, ",");
            CHECK_GREATER_EQUAL(s.size(), 1);

            auto a = util::split<strings>(s[0], ":");
            if(a.size() < 2)
                std::invalid_argument("address must have at least a transport and port. Example: tcp://localhost:10"); 

            address_components c;
            c.address = queue_address;
            c.transport = a[0];
            if(c.transport != "tcp" || c.transport != "udp") 
                std::invalid_argument("address must have a valid transport, `tcp', or `udp'");

            c.host = a[1];
            if(c.host.size() < 3)
                std::invalid_argument("address must have be greater than 2 characters");

            if(c.host[0] != '/' || c.host[1] != '/')
                std::invalid_argument("address must start with `//'");

            c.host = c.host.substr(2);
            c.options = defaults;

            c.port = parse_port(a[2]);

            if(s.size() > 1) 
            {
                auto overrides = parse_options(strings(s.begin() + 1, s.end()));
                c.options.insert(overrides.begin(), overrides.end());
            }

            return c;
        }
    }
}
