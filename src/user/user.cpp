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

#include "user/user.hpp"
#include "util/mencode.hpp"
#include "util/dbc.hpp"

#include <fstream>

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;
namespace u = fire::util;

namespace fire
{
    namespace user
    {
        bool create_home_directory(const std::string& dir)
        {
            if(bf::exists(dir)) return true;

            bf::create_directories(dir);

            return bf::exists(dir);
        }

        u::dict convert(const user_info& u)
        {
            u::dict d;

            d["address"] = u.address();
            d["name"] = u.name();
            d["id"] = u.id();

            return d;
        }

        void convert(const u::dict& d, user_info& u)
        {
            u = {
                d["address"].as_string(),
                d["name"].as_string(),
                d["id"].as_string()
            };
        }

        u::array convert(const users& us)
        {
            u::array a;
            for(auto u : us) 
            { 
                CHECK(u);
                a.add(convert(*u));
            }

            ENSURE_EQUAL(a.size(), us.size());
            return a;
        }

        void convert(const u::array& a, users& us)
        {
            for(auto v : a)
            {
                user_info_ptr n{new user_info};
                convert(v.as_dict(), *n);

                CHECK(n);
                us.push_back(n);
            }

            ENSURE_EQUAL(us.size(), a.size());
        }

        std::ostream& operator<<(std::ostream& out, const user_info& u)
        {
            out << convert(u);
            return out;
        }

        std::istream& operator>>(std::istream& in, user_info& u)
        {
            u::dict d;
            in >> d;

            convert(d, u);
            return in;
        }

        std::ostream& operator<<(std::ostream& out, const users& us)
        {
            out << convert(us);
            return out;
        }

        std::istream& operator>>(std::istream& in, users& u)
        {
            u::array a;
            in >> a;

            convert(a, u);
            return in;
        }

        std::string get_local_user_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "local";
            return l.string();
        }

        std::string get_local_contacts_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "contacts";
            return l.string();
        }

        local_user_ptr load_local_user(const std::string& home_dir)
        {
            create_home_directory(home_dir);

            std::string local_user_file = get_local_user_file(home_dir);
            std::string local_contacts_file = get_local_contacts_file(home_dir);

            std::ifstream info_in(local_user_file.c_str());
            if(!info_in.good()) return {};

            user_info info;
            info_in >> info; 

            users contacts;
            std::ifstream contacts_in(local_contacts_file.c_str());
            if(contacts_in.good()) 
            {
                contacts_in >> contacts;
            }

            local_user_ptr lu{new local_user{info, contacts}};

            ENSURE(lu);
            return lu;
        }

        void save_local_user(const std::string& home_dir, local_user_ptr lu)
        {
            REQUIRE(lu);

            create_home_directory(home_dir);

            std::string local_user_file = get_local_user_file(home_dir);
            std::string local_contacts_file = get_local_contacts_file(home_dir);

            std::ofstream info_out(local_user_file.c_str());
            if(!info_out.good()) 
                throw std::runtime_error{"unable to save `" + local_user_file + "'"};

            info_out << lu->info();

            std::ofstream contacts_out(local_contacts_file.c_str());
            if(!contacts_out.good()) 
                throw std::runtime_error{"unable to save `" + local_contacts_file + "'"};
            
            contacts_out << lu->contacts();
        }
    }
}
