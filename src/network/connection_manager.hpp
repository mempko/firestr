/*
 * Copyright (C) 2012  Maxim Noah Khailo
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

#include "network/message_queue.hpp"
#include "network/boost_asio.hpp"
#include "util/thread.hpp"

#include <string>
#include <map>
#include <vector>

namespace fire 
{
    namespace network 
    {
        typedef std::map<std::string, int> assignment_map; 
        typedef std::vector<boost_asio_queue_ptr> connection_pool;

        class connection_manager
        {
            public:
                connection_manager(size_t size, const std::string& listen_port);

            public:
                boost_asio_queue_ptr connect(const std::string& address);
                boost_asio_queue_ptr get(const std::string& address);

            public:
                bool recieve(util::bytes& b);
                connection* get_socket() const;

            private:
                assignment_map _assignments;
                connection_pool _pool;
                std::string _local_port;
                size_t _next_available;
                std::mutex _mutex;
                boost_asio_queue_ptr _in;
        };
    }
}

#endif
