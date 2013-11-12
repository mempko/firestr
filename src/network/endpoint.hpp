
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
#ifndef FIRESTR_NETWORK_ENDPOINT_H
#define FIRESTR_NETWORK_ENDPOINT_H

#include <string>

namespace fire 
{
    namespace network 
    {
        using port_type = short unsigned int;
        std::string port_to_string(port_type);
        port_type parse_port(const std::string&);

        struct endpoint
        {
            std::string protocol;
            std::string address;
            port_type port;

            bool operator==(const endpoint& o) const
            {
                return protocol == o.protocol && address == o.address && port == o.port;
            }
        };

        using host_port = std::pair<std::string,port_type>;
        host_port parse_host_port(const std::string&);
    }
}
#endif
