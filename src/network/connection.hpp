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
#ifndef FIRESTR_NETWORK_CONNECTION_H
#define FIRESTR_NETWORK_CONNECTION_H

#include "network/endpoint.hpp"
#include "network/util.hpp"
#include "network/message_queue.hpp"
#include "util/thread.hpp"

namespace fire
{
    namespace network
    {
        extern const std::string TCP;
        extern const std::string UDP;
        class connection;

        struct asio_params
        {
            enum endpoint_type { tcp, udp} type;
            enum connect_mode {bind, connect, delayed_connect} mode; 
            std::string uri;
            std::string host;
            port_type port;
            port_type local_port;
            bool block;
            double wait;
            bool track_incoming;
        };

        class connection
        {
            public:
            virtual bool send(const fire::util::bytes& b, bool block = false) = 0;
            virtual endpoint get_endpoint() const = 0;
            virtual bool is_disconnected() const = 0;
        };

        asio_params parse_params(const address_components& c);
        asio_params::endpoint_type determine_type(const std::string& address);
        std::string make_tcp_address(const std::string& host, port_type port, port_type local_port = 0);
        std::string make_udp_address(const std::string& host, port_type port, port_type local_port = 0);
        std::string make_address_str(const endpoint& e);
    }
}

#endif
