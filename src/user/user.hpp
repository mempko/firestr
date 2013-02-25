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

#ifndef FIRESTR_USER_USER_H
#define FIRESTR_USER_USER_H

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "util/thread.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace user
    {
        class user_info
        {
            public:
                user_info() :
                    _address{}, _name{}, _id{} {}

                user_info(
                        const std::string& address, 
                        const std::string& name, 
                        const std::string& id) :
                    _address{address}, _name{name}, _id{id} 
                {
                    REQUIRE_FALSE(address.empty());
                    REQUIRE_FALSE(name.empty());
                    REQUIRE_FALSE(id.empty());
                }

                user_info(const user_info& o) :
                    _address{o._address}, _name{o._name}, _id{o._id} {}

                user_info& operator=(const user_info& o)
                {
                    fire::util::mutex_scoped_lock l(_mutex);
                    if(&o == this) return *this;

                    fire::util::mutex_scoped_lock lo(o._mutex);
                    _name = o._name;
                    _id = o._id;
                    _address = o._address;
                    return *this;
                }

            public:
                std::string name() const;
                std::string id() const;
                std::string address() const;

                void name(const std::string& v);
                void address(const std::string& v);
                void id(const std::string& v);

            private:
                std::string _address;
                std::string _name;
                std::string _id;
                mutable std::mutex _mutex;
        };

        using user_info_ptr = std::shared_ptr<user_info>;
        using user_info_wptr = std::weak_ptr<user_info>;
        using users = std::vector<user_info_ptr>;
        using user_map = std::map<std::string, size_t>;

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
                greet_server() : _host(), _port(){}

                greet_server(const greet_server& o) : 
                    _host{o._host}, _port{o._port}{}

                greet_server(const std::string& host, const std::string& port) : 
                    _host{host}, _port{port}{}

            public:
                std::string host() const; 
                std::string port() const;
                void host(const std::string& host); 
                void port(const std::string& port);

            private:
                std::string _host;
                std::string _port;
                mutable std::mutex _mutex;
        };

        using greet_servers = std::vector<greet_server>;

        class local_user
        {
            public:
                local_user(
                        const user_info& i, 
                        const contact_list& c,
                        const greet_servers& g);

                local_user(const std::string& name); 

            public:
                const user_info& info() const { return _info;}
                user_info& info() { return _info;}

                const contact_list& contacts() const { return _contacts;}
                contact_list& contacts() { return _contacts;}

                const greet_servers& greeters() const { return _greet_servers;}
                greet_servers& greeters() { return _greet_servers;}

            private:
                user_info _info;
                contact_list _contacts;
                greet_servers _greet_servers;
        };

        typedef std::shared_ptr<local_user> local_user_ptr;
        typedef std::weak_ptr<local_user> local_user_wptr;

        //load and save local user info to disk
        local_user_ptr load_user(const std::string& home_dir);
        void save_user(const std::string& home_dir, const local_user&);

        //loads and saves a contact
        user_info_ptr load_contact(const std::string& file);
        void save_contact(const std::string& file, const user_info&);

        //serialization functions
        std::ostream& operator<<(std::ostream& out, const user_info& u);
        std::istream& operator>>(std::istream& in, user_info& u);
    }
}

#endif
