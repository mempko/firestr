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
#include "message/message.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace fire
{
    namespace message
    {
        std::ostream& operator<<(std::ostream& o, const message& m)
        {
            const metadata& meta = m.meta;
            const util::bytes& data = m.data;

            //write out type
            util::bytes mt = util::to_bytes(meta.type);
            o << mt;

            //write out extra metadata.
            //we encode it into a byte string so we can 
            //skip quickly
            std::stringstream ms;

            util::array to;
            for(auto s: meta.to) to.add(s);

            util::array from;
            for(auto s: meta.from) from.add(s);

            ms << to << from << meta.extra;
            util::bytes mb = util::to_bytes(ms.str());
            o << mb;

            //write out data
            o << m.data;

            return o;
        }

        std::istream& operator>>(std::istream& i, message& m)
        {
            metadata& meta = m.meta;
            util::bytes& data = m.data;

            //read type
            util::bytes mt;
            i >> mt;
            meta.type = util::to_str(mt);

            //read extra metadata
            util::bytes mb;
            i >> mb;
            std::stringstream ms(util::to_str(mb));
            util::array to;
            util::array from;
            ms >> to >> from >> meta.extra; 

            for(auto s : to) meta.to.push_back(s.as_string());
            for(auto s : from) meta.from.push_back(s.as_string());

            //read data
            i >> m.data;

            return i;
        }

        std::string external_address(const std::string& host, const std::string& port)
        {
            return "zmq,tcp://" + host + ":" + port;
        }

        std::string external_address(const std::string& host_port)
        {
            return "zmq,tcp://" + host_port;
        }
    }
}

namespace std
{
        std::ostream& operator<<(std::ostream& o, fire::message::address a)
        {
            o << a.front();
            a.pop_front();
            for(auto p : a) o << ':' << p;

            return o;
        }
}
