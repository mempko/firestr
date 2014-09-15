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
            return "udp://" + host + ":" + port;
        }

        std::string external_address(const std::string& host_port)
        {
            return "udp://" + host_port;
        }

        std::string to_str(metadata::encryption_type t)
        {
            switch(t)
            {
                case metadata::symmetric: return "symmetric";
                case metadata::asymmetric: return "asymmetric";
                case metadata::plaintext: return "plaintext";
            }
            return "unknown";
        }

        bool is_local(const message& m)
        {
            return m.meta.source == metadata::local;
        }

        bool is_remote(const message& m)
        {
            return m.meta.source == metadata::remote;
        }

        bool is_symmetric(const message& m)
        {
            return m.meta.encryption == metadata::symmetric;
        }

        bool is_asymmetric(const message& m)
        {
            return m.meta.encryption == metadata::asymmetric;
        }

        bool is_plaintext(const message& m)
        {
            return m.meta.encryption == metadata::plaintext;
        }

        void expect_local(const message& m)
        {
            if(is_local(m)) return;
            throw std::runtime_error{"expected message to be local but remote"};
        }

        void expect_remote(const message& m)
        {
            if(is_remote(m)) return;
            throw std::runtime_error{"expected message to be remote but got local"};
        }

        void expect_symmetric(const message& m)
        {
            if(is_symmetric(m)) return;
            throw std::runtime_error{"expected message with symmetric encryption but got `" + to_str(m.meta.encryption) + "'"};
        }

        void expect_asymmetric(const message& m)
        {
            if(is_asymmetric(m)) return;
            throw std::runtime_error{"expected message with asymmetric encryption but got `" + to_str(m.meta.encryption) + "'"};
        }

        void expect_plaintext(const message& m)
        {
            if(is_plaintext(m)) return;
            throw std::runtime_error{"expected message with plaintext encryption but got `" + to_str(m.meta.encryption) + "'"};
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
