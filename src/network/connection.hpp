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
#ifndef FIRESTR_NETWORK_CONNECTION_H
#define FIRESTR_NETWORK_CONNECTION_H

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
        struct endpoint
        {
            std::string protocol;
            std::string address;
            std::string port;
        };

        struct asio_params
        {
            enum endpoint_type { tcp, udp} type;
            enum connect_mode {bind, connect, delayed_connect} mode; 
            std::string uri;
            std::string host;
            std::string port;
            std::string local_port;
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
        std::string make_tcp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        std::string make_udp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        std::string make_address_str(const endpoint& e);
    }
}

#endif
