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
#ifndef FIRESTR_NETWORK_CONNECTION_MANAGER_H
#define FIRESTR_NETWORK_CONNECTION_MANAGER_H

#include "network/tcp_queue.hpp"
#include "network/udp_queue.hpp"
#include "util/thread.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace fire 
{
    namespace network 
    {
        using assignment_map = std::unordered_map<std::string, int>; 
        using tcp_connection_pool = std::vector<tcp_queue_ptr>;
        using connection_map = std::unordered_map<std::string, connection*>; 

        class connection_manager
        {
            public:
                connection_manager(size_t size, port_type listen_port);

            public:
                bool receive(endpoint& ep, util::bytes& b);
                bool send(const std::string& to, const util::bytes& b);
                bool is_disconnected(const std::string& addr);

            private:
                tcp_queue_ptr connect(const std::string& address);
                void teardown_and_repool_tcp_connections();
                void create_udp_endpoint();
                void create_tcp_endpoint();
                void create_tcp_pool();

            private:
                enum receive_state { 
                    IN_UDP1, 
                    IN_UDP2, 
                    IN_UDP3, 
                    IN_UDP4, 
                    IN_UDP5, 
                    IN_UDP6, 
                    IN_UDP7, 
                    IN_UDP8, 
                    IN_TCP, 
                    OUT_TCP, 
                    DONE};

                void transition_udp_state();
            private:

                receive_state _rstate;
                assignment_map _out;
                tcp_connection_pool _pool;
                port_type _local_port;
                size_t _next_available;
                std::mutex _mutex;
                tcp_queue_ptr _in;
                udp_queue_ptr _udp_con;

                connection_map _in_connections;
        };
    }
}

#endif
