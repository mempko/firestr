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

#include "network/connection.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <sstream> 

namespace fire
{
    namespace network
    {
        const std::string TCP = "tcp"; 
        const std::string UDP = "udp"; 

        asio_params::connect_mode determine_connection_mode(const queue_options& o)
        {
            asio_params::connect_mode m = asio_params::connect;

            if(o.count("bnd")) m = asio_params::bind;
            else if(o.count("con")) m = asio_params::connect;
            else if(o.count("dcon")) m = asio_params::delayed_connect;

            return m;
        }

        asio_params::endpoint_type determine_type(const std::string& t)
        {
            asio_params::endpoint_type m = asio_params::udp;
            auto s = t.substr(0,3);
            if(s == TCP) m = asio_params::tcp;
            else if(s == UDP) m = asio_params::udp;
            else LOG << "unknown transport: `" << s << "', using UDP " << std::endl;

            return m;
        }

        asio_params parse_params(const address_components& c)
        {
            const auto& o = c.options;

            asio_params p;
            p.type = determine_type(c.transport);
            p.mode = determine_connection_mode(o);
            p.uri = c.address;
            p.host = c.host;
            p.port = c.port;
            p.local_port = parse_port(get_opt(o, "local_port", std::string("0")));
            p.block = get_opt(o, "block", 0);
            p.wait = get_opt(o, "wait", 0);
            p.track_incoming = get_opt(o, "track_incoming", 0);

            return p;
        }

        std::string make_pro_address(
                const std::string& proto, 
                const std::string& host, 
                port_type port, 
                port_type local_port)
        {
            std::stringstream s;
            s << proto << "://" << host << ":" << port; 
            if(local_port > 0) s << ",local_port=" << local_port;
            return s.str();
        }

        std::string make_tcp_address(const std::string& host, port_type port, port_type local_port)
        {
            return make_pro_address(TCP, host, port, local_port);
        }
        std::string make_udp_address(const std::string& host, port_type port, port_type local_port)
        {
            return make_pro_address(UDP, host, port, local_port);
        }

        std::string make_address_str(const endpoint& e)
        {
            std::stringstream s; s << e.protocol << "://" << e.address << ":" << e.port;
            return s.str();
        }

    }
}
