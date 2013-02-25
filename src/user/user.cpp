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
#include "util/uuid.hpp"
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
            u = 
            {
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

        u::dict from_greet_server(const greet_server& g)
        {
            u::dict d;

            d["host"] = g.host();
            d["port"] = g.port();

            return d;
        }

        greet_server to_greet_server(const u::dict& d)
        {
            greet_server g{
                d["host"].as_string(),
                d["port"].as_string(),
            };
            return g;
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

        std::ostream& operator<<(std::ostream& out, const greet_servers& gs)
        {
            out << u::to_array(gs, from_greet_server);
            return out;
        }

        std::istream& operator>>(std::istream& in, greet_servers& gs)
        {
            u::array a;
            in >> a;

            u::from_array(a, gs, to_greet_server);
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

        std::string get_local_greeters_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "greeters";
            return l.string();
        }

        local_user_ptr load_user(const std::string& home_dir)
        {
            auto local_user_file = get_local_user_file(home_dir);
            auto local_contacts_file = get_local_contacts_file(home_dir);
            auto local_greaters_file = get_local_greeters_file(home_dir);

            std::ifstream info_in(local_user_file.c_str());
            if(!info_in.good()) return {};

            user_info info;
            info_in >> info; 

            users us;
            std::ifstream contacts_in(local_contacts_file.c_str());
            if(contacts_in.good()) 
            {
                contacts_in >> us;
            }

            greet_servers greeters;
            std::ifstream greeters_in(local_greaters_file.c_str());
            if(greeters_in.good()) 
            {
                greeters_in >> greeters;
            }

            contact_list contacts{us};
            local_user_ptr lu{new local_user{info, contacts, greeters}};

            ENSURE(lu);
            return lu;
        }

        void save_user(const std::string& home_dir, const local_user& lu)
        {
            create_home_directory(home_dir);

            auto local_user_file = get_local_user_file(home_dir);
            auto local_contacts_file = get_local_contacts_file(home_dir);
            auto local_greaters_file = get_local_greeters_file(home_dir);

            {
                std::ofstream info_out(local_user_file.c_str());
                if(!info_out.good()) 
                    throw std::runtime_error{"unable to save `" + local_user_file + "'"};

                info_out << lu.info();
            }

            {
                std::ofstream contacts_out(local_contacts_file.c_str());
                if(!contacts_out.good()) 
                    throw std::runtime_error{"unable to save `" + local_contacts_file + "'"};

                contacts_out << lu.contacts().list();
            }

            {
                std::ofstream greeters_out(local_greaters_file.c_str());
                if(!greeters_out.good()) 
                    throw std::runtime_error{"unable to save `" + local_greaters_file + "'"};

                greeters_out << lu.greeters();
            }
        }

        user_info_ptr load_contact(const std::string& file)
        {
            std::ifstream in(file.c_str());
            if(!in.good()) return {};

            user_info_ptr u{new user_info};
            in >> *u;
            return u;
        }

        void save_contact(const std::string& file, const user_info& u)
        {
            std::ofstream out(file.c_str());
            if(!out.good()) return;

            out << u;
        }

        local_user::local_user(
                const user_info& i, 
                const contact_list& c,
                const greet_servers& g) : 
            _info{i},
            _contacts{c},
            _greet_servers{g}
        {
        }

        local_user::local_user(const std::string& name) : 
            _info{"local", name, util::uuid()}, 
            _contacts{},
            _greet_servers{}
        {
            REQUIRE_FALSE(name.empty());

            INVARIANT_EQUAL(_info.address(), "local");
            INVARIANT_FALSE(_info.name().empty());
            INVARIANT_FALSE(_info.id().empty());
            ENSURE(_contacts.empty());
        }

        contact_list::contact_list(const users& cs)
        {
            for(auto c : cs)
            {
                CHECK(c);
                add(c);
            }

            ENSURE_EQUAL(_list.size(), _map.size());
            ENSURE_LESS_EQUAL(_list.size(), cs.size());
        }

        contact_list::contact_list(const contact_list& o)
        {
            u::mutex_scoped_lock l(o._mutex);
            _list = o._list;
            _map = o._map;
        }

        contact_list::contact_list() : _list{}, _map{} {}

        users contact_list::list() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _list;
        }

        bool contact_list::add(user_info_ptr c)
        {
            u::mutex_scoped_lock l(_mutex);

            REQUIRE(c);
            if(_map.count(c->id())) return false;

            _list.push_back(c);
            _map[c->id()] = _list.size() - 1;

            ENSURE_FALSE(_list.empty());
            ENSURE_EQUAL(_list.back(), c);
            ENSURE_EQUAL(_list[_map[c->id()]].get(), c.get());

            return true;
        }

        bool contact_list::remove(user_info_ptr c)
        {
            u::mutex_scoped_lock l(_mutex);

            REQUIRE(c);

            auto i = _map.find(c->id());
            if(i == _map.end()) return false;
            
            _list.erase(_list.begin() + i->second);
            _map.erase(i);

            ENSURE_EQUAL(_map.count(c->id()), 0);
            return true;
        }

        user_info_ptr contact_list::by_id(const std::string& id) const
        {
            u::mutex_scoped_lock l(_mutex);

            auto p = _map.find(id);
            return p != _map.end() ? _list[p->second] : 0; 
        }

        user_info_ptr contact_list::get(size_t i) const
        {
            u::mutex_scoped_lock l(_mutex);

            if(i >= _list.size()) return nullptr;

            auto c = _list[i];
            ENSURE(c);
            return c;
        }

        bool contact_list::empty() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _list.empty();
        }

        size_t contact_list::size() const
        {
            u::mutex_scoped_lock l(_mutex);

            INVARIANT_EQUAL(_list.size(), _map.size());
            return _list.size();
        }

        void contact_list::clear()
        {
            u::mutex_scoped_lock l(_mutex);

            _list.clear();
            _map.clear();

            ENSURE(_list.empty());
            ENSURE(_map.empty());
        }

        std::string user_info::name() const 
        { 
            u::mutex_scoped_lock l(_mutex);
            return _name;
        }

        std::string user_info::id() const 
        {
            u::mutex_scoped_lock l(_mutex);
            return _id;
        }

        std::string user_info::address() const 
        {
            u::mutex_scoped_lock l(_mutex);
            return _address;
        }

        void user_info::name(const std::string& v) 
        {
            u::mutex_scoped_lock l(_mutex);
            _name = v;
        }

        void user_info::address(const std::string& v) 
        {
            u::mutex_scoped_lock l(_mutex);
            _address = v;
        }

        void user_info::id(const std::string& v) 
        {
            u::mutex_scoped_lock l(_mutex);
            _id = v;
        }

        std::string greet_server::host() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _host;
        }

        std::string greet_server::port() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _port;
        }

        void greet_server::host(const std::string& v)
        {
            u::mutex_scoped_lock l(_mutex);
            _host = v;
        }

        void greet_server::port(const std::string& v)
        {
            u::mutex_scoped_lock l(_mutex);
            _port = v;
        }
    }
}
