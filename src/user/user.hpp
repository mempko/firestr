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

            public:
                const std::string& name() const { return _name;}
                const std::string& id() const { return _id;}
                const std::string& address() const { return _address;}

                void name(const std::string& v) { _name = v;}
                void address(const std::string& v) { _address = v;}
                void id(const std::string& v) { _id = v;}

            private:
                std::string _address;
                std::string _name;
                std::string _id;
        };

        typedef std::shared_ptr<user_info> user_info_ptr;
        typedef std::weak_ptr<user_info> user_info_wptr;
        typedef std::vector<user_info_ptr> users;
        typedef std::map<std::string, user_info_ptr> user_map;

        class local_user
        {
            public:
                local_user(
                        const user_info& i,
                        const users& c) :
                    _info{i}, _contacts{c} {}

            public:
                const user_info& info() const { return _info;}
                const users& contacts() const { return _contacts;}

            private:
                user_info _info;
                users _contacts;
        };

        typedef std::shared_ptr<local_user> local_user_ptr;
        typedef std::weak_ptr<local_user> local_user_wptr;

        //load and save local user info to disk
        local_user_ptr load_user(const std::string& home_dir);
        void save_user(const std::string& home_dir, local_user_ptr);
    }
}

#endif
