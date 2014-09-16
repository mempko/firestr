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
#ifndef FIRESTR_NETWORK_MESSAGE_QUEUE_H
#define FIRESTR_NETWORK_MESSAGE_QUEUE_H

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>

#include "network/endpoint.hpp"
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

                bool receive(std::string& s)
                {
                    util::bytes b;
                    if(receive(b)) 
                    {
                        s = util::to_str(b);
                        return true;
                    }
                    return false;
                }
                
                virtual bool send(const util::bytes&) = 0;
                virtual bool receive(util::bytes&) = 0;
        };

        using message_queue_ptr = std::shared_ptr<message_queue>;
        using queue_options = std::map<std::string, std::string>;

        template<class t>
            t get_opt(const queue_options& o, const std::string& k, t def)
            {
                auto i = o.find(k);
                if(i != o.end()) return boost::lexical_cast<t>(i->second);
                return def;
            }

        struct address_components
        {
            std::string address; 
            std::string transport; 
            std::string host;
            port_type port;
            queue_options options;
        };

        address_components parse_address(
                const std::string& queue_address, 
                const queue_options& defaults = queue_options());

    }
}

#endif
