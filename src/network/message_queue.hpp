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
#ifndef FIRESTR_NETWORK_MESSAGE_QUEUE_H
#define FIRESTR_NETWORK_MESSAGE_QUEUE_H

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <map>

#include "util/bytes.hpp"
#include "util/dbc.hpp"

namespace fire 
{
    namespace network 
    {
        class message_queue
        {
            public:
                bool send(const std::string& s)
                {
                    return send(util::to_bytes(s));
                }

                bool recieve(std::string& s)
                {
                    util::bytes b;
                    if(recieve(b)) 
                    {
                        s = util::to_str(b);
                        return true;
                    }
                    return false;
                }
                
                virtual bool send(const util::bytes&) = 0;
                virtual bool recieve(util::bytes&) = 0;
        };

        typedef std::shared_ptr<message_queue> message_queue_ptr;
        typedef std::map<std::string, std::string> queue_options;

        message_queue_ptr create_message_queue(
                const std::string& queue_address, 
                const queue_options& default_options = queue_options());
    }
}

#endif
