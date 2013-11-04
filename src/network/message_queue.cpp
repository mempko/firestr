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

        host_port parse_host_port(const std::string& a)
        {
            auto s = util::split<strings>(a, ":");
            if(s.size() != 2) std::invalid_argument("address must have host and port."); 
            return std::make_pair(s[0], s[1]);
        }

        address_components parse_address(
                const std::string& queue_address, 
                const queue_options& defaults)
        {
            auto s = util::split<strings>(queue_address, ",");
            if(s.size() < 1) std::invalid_argument("address must have at least a transport and port. Example: tcp://localhost:10"); 

            auto a = util::split<strings>(s[0], ":");

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

            c.port = a[2];

            try
            {
                int nport = boost::lexical_cast<int>(c.port);
                if(nport <= 0) std::invalid_argument("invalid port");
            }
            catch(...)
            {
                std::invalid_argument("port must be a number");
            }

            if(s.size() > 1) 
            {
                auto overrides = parse_options(strings(s.begin() + 1, s.end()));
                c.options.insert(overrides.begin(), overrides.end());
            }

            return c;
        }
    }
}
