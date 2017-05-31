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
#ifndef FIRESTR_MESSAGE_MESSAGE_H
#define FIRESTR_MESSAGE_MESSAGE_H

#include <string>
#include <iostream>
#include <deque>

#include "util/serialize.hpp"
#include "util/mencode.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace message
    {
        using address = std::deque<std::string>;
        struct metadata
        {
            std::string type;
            address to;
            address from;
            util::dict extra;
            enum encryption_type { conversation, asymmetric, symmetric, plaintext};
            enum source_type {local, remote};
            source_type source = source_type::local;
            encryption_type encryption = encryption_type::conversation;
            bool robust = true;
        };

        struct message
        {
            metadata meta; 
            util::bytes data;
        };

        std::ostream& operator<<(std::ostream&, const message&);
        std::istream& operator>>(std::istream&, message&);

        std::string external_address(const std::string& host, const std::string& port);
        std::string external_address(const std::string& host_port);

        bool is_local(const message& m);
        bool is_remote(const message& m);
        bool is_symmetric(const message& m);
        bool is_asymmetric(const message& m);
        bool is_plaintext(const message& m);

        void expect_local(const message& m);
        void expect_remote(const message& m);
        void expect_symmetric(const message& m);
        void expect_asymmetric(const message& m);
        void expect_plaintext(const message& m);



        /**
         * Converts your data structure into a message.
         * Datastructure must have util::serialize method using f_serialize
         */
        template <class C>
            struct as_message
            {
                const std::string type;
                std::string from_id;
                std::string from_ip;
                int from_port = 0;

                explicit as_message(const std::string& t) : type(t) {}

                message to_message() const
                {
                    message m;
                    m.meta.type = type;
                    if(!from_id.empty()) m.meta.extra["from_id"] = from_id;
                    if(!from_ip.empty()) m.meta.extra["from_ip"] = from_ip;
                    if(from_port != 0) m.meta.extra["from_port"] = from_port;

                    //serialize structure
                    const C& self = reinterpret_cast<const C&>(*this);
                    util::serialize(m.data, self);

                    ENSURE_EQUAL(m.meta.type, type);
                    return m;
                }

                void from_message(const message& m)
                {
                    REQUIRE_EQUAL(m.meta.type, type);

                    if(m.meta.extra.has("from_id")) 
                        from_id = m.meta.extra["from_id"].as_string();
                    if(m.meta.extra.has("from_ip")) 
                        from_ip = m.meta.extra["from_ip"].as_string();
                    if(m.meta.extra.has("from_port")) 
                        from_port = m.meta.extra["from_port"];

                    C& self = reinterpret_cast<C&>(*this);
                    util::deserialize(m.data, self);
                }
            };

    }

}

namespace std
{
    std::ostream& operator<<(std::ostream&, fire::message::address);
}

#define f_message(c) struct c : fire::message::as_message<c>
#define f_message_init(c, ct) c() : fire::message::as_message<c>(ct) {}
#endif 
