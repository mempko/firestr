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
 */

#include "user/user.hpp"
#include "util/mencode.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

#include <fstream>
#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;
namespace u = fire::util;
namespace n = fire::network;
namespace sc = fire::security;

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
            d["pkey"] = u.key().key();

            return d;
        }

        void convert(const u::dict& d, user_info& u)
        {
            u = 
            {
                d["address"].as_string(),
                d["name"].as_string(),
                d["id"].as_string(),
                d["pkey"].as_string()
            };
        }

        u::dict from_user_info(const user_info& u)
        {
            return convert(u);
        }

        user_info to_user_info(const u::dict& d)
        {
            user_info r;
            convert(d, r);
            return r;
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
            d["port"] = static_cast<int>(g.port());
            d["pub_key"] = g.public_key();

            return d;
        }

        greet_server to_greet_server(const u::dict& d)
        {
            greet_server g{
                d["host"].as_string(),
                d["port"].as_int(),
                d["pub_key"].as_string(),
            };
            return g;
        }

        bool contact_introduction::operator==(const contact_introduction& o) const
        {
            if(this == &o) return true;
            return contact.id() == o.contact.id();
        }

        u::dict from_introduction(const contact_introduction& i)
        {
            u::dict d;

            d["from_id"] = i.from_id;
            d["greeter"] = i.greeter;
            d["message"] = i.message;
            d["contact"] = from_user_info(i.contact);

            return d;
        }

        contact_introduction to_introduction(const u::dict& d)
        {
            contact_introduction i{
                d["from_id"].as_string(),
                d["greeter"].as_string(),
                d["message"].as_string(),
                to_user_info(d["contact"].as_dict()),
            };
            return i;
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

        std::ostream& operator<<(std::ostream& out, const contact_introductions& is)
        {
            out << u::to_array(is, from_introduction);
            return out;
        }

        std::istream& operator>>(std::istream& in, contact_introductions& is)
        {
            u::array a;
            in >> a;

            u::from_array(a, is, to_introduction);
            return in;
        }

        std::string get_local_user_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "local";
            return l.string();
        }

        std::string get_local_private_key_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "private_key";
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

        std::string get_local_introductions_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "introductions";
            return l.string();
        }

        std::string get_local_port_file(const bf::path& home_dir)
        {
            bf::path l = home_dir / "port";
            return l.string();
        }

        bool user_created(const std::string& home_dir)
        {
            return bf::exists(get_local_user_file(home_dir));
        }

        local_user_ptr load_user(const std::string& home_dir, const std::string& passphrase)
        {
            auto local_user_file = get_local_user_file(home_dir);
            auto local_prv_key_file = get_local_private_key_file(home_dir);
            auto local_contacts_file = get_local_contacts_file(home_dir);
            auto local_greaters_file = get_local_greeters_file(home_dir);
            auto local_introductions_file = get_local_introductions_file(home_dir);

            //load user info
            user_info info;
            if(!u::load_from_file(local_user_file, info)) return {};

            //load private key
            std::ifstream key_in(local_prv_key_file.c_str());
            if(!key_in.good()) return {};

            auto prv_key = sc::decode_private_key(key_in, passphrase);
            CHECK(prv_key);

            //load other user information
            users us;
            u::load_from_file(local_contacts_file, us);

            greet_servers greeters;
            u::load_from_file(local_greaters_file, greeters);

            contact_introductions introductions;
            u::load_from_file(local_introductions_file, introductions);

            contact_list contacts{us};
            local_user_ptr lu{new local_user{info, contacts, greeters, introductions, prv_key}};

            ENSURE(lu);
            return lu;
        }

        void save_user(const std::string& home_dir, const local_user& lu)
        {
            create_home_directory(home_dir);

            auto local_user_file = get_local_user_file(home_dir);
            auto local_prv_key_file = get_local_private_key_file(home_dir);
            auto local_contacts_file = get_local_contacts_file(home_dir);
            auto local_greaters_file = get_local_greeters_file(home_dir);
            auto local_introductions_file = get_local_introductions_file(home_dir);

            u::save_to_file(local_user_file, lu.info());

            if(!bf::exists(local_prv_key_file))
            {
                std::ofstream key_out(local_prv_key_file.c_str());
                if(!key_out.good()) 
                    throw std::runtime_error{"unable to save `" + local_prv_key_file + "'"};

                sc::encode(key_out, lu.private_key());
            }

            u::save_to_file(local_contacts_file, lu.contacts().list());
            u::save_to_file(local_greaters_file, lu.greeters());
            u::save_to_file(local_introductions_file, lu.introductions());
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
                const greet_servers& g,
                const contact_introductions& is,
                sc::private_key_ptr pk) : 
            _info{i},
            _contacts{c},
            _greet_servers{g},
            _introductions{is},
            _prv_key{pk}
        {
            REQUIRE(pk);
            INVARIANT(_prv_key);
        }

        local_user::local_user(const std::string& name, sc::private_key_ptr pk) : 
            _info{"local", name, util::uuid(), pk->public_key()}, 
            _contacts{},
            _greet_servers{},
            _prv_key{pk}
        {
            REQUIRE(pk);
            REQUIRE_FALSE(name.empty());

            INVARIANT_EQUAL(_info.address(), "local");
            INVARIANT_FALSE(_info.name().empty());
            INVARIANT_FALSE(_info.id().empty());
            INVARIANT(_contacts.empty());
            INVARIANT(_prv_key);
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

        bool contact_list::has(const std::string& id) const
        {
            u::mutex_scoped_lock l(_mutex);
            return _map.count(id) != 0;
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

        const sc::public_key& user_info::key() const 
        {
            u::mutex_scoped_lock l(_mutex);
            return _pkey;
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

        void user_info::key(const sc::public_key& v) 
        {
            u::mutex_scoped_lock l(_mutex);
            _pkey = v;
        }

        std::string greet_server::host() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _host;
        }

        n::port_type greet_server::port() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _port;
        }

        std::string greet_server::public_key() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _key;
        }

        void greet_server::host(const std::string& v)
        {
            u::mutex_scoped_lock l(_mutex);
            _host = v;
        }

        void greet_server::port(n::port_type v)
        {
            u::mutex_scoped_lock l(_mutex);
            _port = v;
        }

        void greet_server::public_key(const std::string& v)
        {
            u::mutex_scoped_lock l(_mutex);
            _key = v;
        }

        greet_server& greet_server::operator=(const greet_server& o)
        {
            if(&o == this) return *this;

            u::mutex_scoped_lock l(_mutex);
            _host = o.host();
            _port = o.port();
            _key = o.public_key();
            return *this;
        }

        network::port_type load_port(const std::string& home_dir)
        {
            auto local_port_file = get_local_port_file(home_dir);
            if(!bf::exists(local_port_file)) return 0;

            std::ifstream pin(local_port_file.c_str());
            if(!pin.good()) return 0;

            network::port_type r = 0;
            pin >> r;
            return r;
        }

        void save_port(const std::string& home_dir, network::port_type p)
        {
            auto local_port_file = get_local_port_file(home_dir);
            std::ofstream po(local_port_file.c_str());
            if(!po.good()) return;
            po << p;
        }
    }
}
