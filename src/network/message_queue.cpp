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

#include "network/message_queue.hpp"
#include "network/zeromq_queue.hpp"

#include "util/string.hpp"

#include <vector>

namespace fire
{
    namespace network
    {
        namespace
        {
            const std::string SPLIT_CHAR = ",";
        }

        typedef std::vector<std::string> strings;

        message_type determine_type(const std::string type)
        {
            message_type t;
            if(type == "zmq") t = zeromq; 
            else std::invalid_argument("Queue type `" + type + "' is not valid. Currently zmq is supported");
            return t;
        }

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
            auto s = util::split<strings>(queue_address, SPLIT_CHAR);
            if(s.size() < 2) std::invalid_argument("Queue address must have at least a type and location. Example: zmq,http://localhost:10"); 
            address_components c;
            c.queue_address = queue_address;
            c.type = determine_type(s[0]);
            c.location = s[1];
            c.options = defaults;

            if(s.size() > 2) 
            {
                auto overrides = parse_options(strings(s.begin() + 2, s.end()));
                c.options.insert(overrides.begin(), overrides.end());
            }

            return c;
        }

        message_queue_ptr create_message_queue(
                const std::string& queue_address, 
                const queue_options& defaults)
        {
            auto c = parse_address(queue_address, defaults); 
            message_queue_ptr p;

            switch(c.type)
            {
                case zeromq: p = create_zmq_message_queue(c); break;
                default: CHECK(false && "missed case");
            }

            ENSURE(p);
            return p;
        }

        std::string make_zmq_address(const std::string& host, const std::string& port)
        {
            return "zmq,tcp://" + host + ":" + port;
        }
    }
}
