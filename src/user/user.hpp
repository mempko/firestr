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

#ifndef FIRESTR_USER_USER_H
#define FIRESTR_USER_USER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "network/endpoint.hpp"
#include "security/security.hpp"

#include "util/dbc.hpp"
#include "util/mencode.hpp"
#include "util/thread.hpp"

namespace fire
{
    namespace user
    {
        class user_info
        {
            public:
                user_info() :
                    _address{}, _name{}, _id{}, _pkey{} {}

                user_info(
                        const std::string& address, 
                        const std::string& name, 
                        const std::string& id,
                        const security::public_key& pub_key) :
                    _address{address}, _name{name}, _id{id}, _pkey{pub_key} 
                {
                    REQUIRE_FALSE(address.empty());
                    REQUIRE_FALSE(name.empty());
                    REQUIRE_FALSE(id.empty());
                    REQUIRE_FALSE(pub_key.key().empty());
                }

                user_info(const user_info& o) :
                    _address{o._address}, _name{o._name}, _id{o._id}, _pkey{o._pkey} {}

                user_info& operator=(const user_info& o)
                {
                    fire::util::mutex_scoped_lock l(_mutex);
                    if(&o == this) return *this;

                    fire::util::mutex_scoped_lock lo(o._mutex);
                    _name = o._name;
                    _id = o._id;
                    _address = o._address;
                    _pkey = o._pkey;
                    return *this;
                }

            public:
                std::string name() const;
                std::string id() const;
                std::string address() const;
                const security::public_key& key() const;


                void name(const std::string& v);
                void address(const std::string& v);
                void id(const std::string& v);
                void key(const security::public_key& v);

            private:
                std::string _address;
                std::string _name;
                std::string _id;
                security::public_key _pkey;
                mutable std::mutex _mutex;
        };

        using user_info_ptr = std::shared_ptr<user_info>;
        using user_info_wptr = std::weak_ptr<user_info>;
        using users = std::vector<user_info_ptr>;
        using user_map = std::unordered_map<std::string, size_t>;

        class contact_list
        {
            public:
                contact_list(const users&);
                contact_list(const contact_list&);
                contact_list();

            public:
                users list() const;
                bool add(user_info_ptr);
                bool remove(user_info_ptr);
                bool has(const std::string& id) const;
                user_info_ptr by_id(const std::string& id) const;
                user_info_ptr get(size_t) const;
                
            public:
                bool empty() const;
                size_t size() const;
                void clear();

            private:
                users _list;
                user_map _map;
                mutable std::mutex _mutex;
        };

        class greet_server
        {
            public:
                greet_server() : _host(), _port(), _key(){}

                greet_server(const greet_server& o) : 
                    _host{o._host}, _port{o._port}, _key{o._key}{}

                greet_server(const std::string& host, network::port_type port, const std::string& key) : 
                    _host{host}, _port{port}, _key{key}{}

                greet_server& operator=(const greet_server& o);

            public:
                std::string host() const; 
                network::port_type port() const;
                std::string public_key() const;
                void host(const std::string& host); 
                void port(network::port_type port);
                void public_key(const std::string& key);

            private:
                std::string _host;
                network::port_type _port;
                std::string _key;
                mutable std::mutex _mutex;
        };

        using greet_servers = std::vector<greet_server>;

        struct contact_introduction
        {
            std::string from_id;
            std::string greeter;
            std::string message;
            user_info contact;
            bool operator==(const contact_introduction&) const;
        };
        using contact_introductions = std::vector<contact_introduction>;
        contact_introduction to_introduction(const util::dict&);
        util::dict from_introduction(const contact_introduction&);

        class local_user
        {
            public:
                local_user(
                        const user_info& i, 
                        const contact_list& c,
                        const greet_servers& g,
                        const contact_introductions& is,
                        security::private_key_ptr);

                local_user(const std::string& name, security::private_key_ptr); 

            public:
                const user_info& info() const { return _info;}
                user_info& info() { return _info;}

                const contact_list& contacts() const { return _contacts;}
                contact_list& contacts() { return _contacts;}

                const greet_servers& greeters() const { return _greet_servers;}
                greet_servers& greeters() { return _greet_servers;}

                const contact_introductions& introductions() const { return _introductions;}
                contact_introductions& introductions() { return _introductions;}

                const security::private_key& private_key() const { return *_prv_key;}

            private:
                user_info _info;
                contact_list _contacts;
                greet_servers _greet_servers;
                contact_introductions _introductions;
                security::private_key_ptr _prv_key;
        };

        using local_user_ptr = std::shared_ptr<local_user>;
        using local_user_wptr = std::weak_ptr<local_user>;


        //load and save local user info to disk
        bool user_created(const std::string& home_dir);
        local_user_ptr load_user(const std::string& home_dir, const std::string& passphrase);
        void save_user(const std::string& home_dir, const local_user&);

        //loads and saves a contact
        user_info_ptr load_contact(const std::string& file);
        void save_contact(const std::string& file, const user_info&);

        //load and save cached port
        network::port_type load_port(const std::string& home_dir);
        void save_port(const std::string& home_dir, network::port_type);

        //serialization functions
        std::ostream& operator<<(std::ostream& out, const user_info& u);
        std::istream& operator>>(std::istream& in, user_info& u);
    }
}

#endif
