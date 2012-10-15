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

#include <string>
#include <cstdlib>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>

#include "network/connection_manager.hpp"
#include "message/message.hpp"
#include "messages/greeter.hpp"
#include "util/thread.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"

namespace po = boost::program_options;
namespace ip = boost::asio::ip;
namespace n = fire::network;
namespace m = fire::message;
namespace ms = fire::messages;
namespace u = fire::util;

namespace
{
    const size_t THREAD_SLEEP = 100; //in milliseconds
    const size_t POOL_SIZE = 10; //small pool size for now
}

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    const std::string host = ip::host_name();
    const std::string port = "7070";

    d.add_options()
        ("help", "prints help")
        ("host", po::value<std::string>()->default_value(host), "host/ip of this server") 
        ("port", po::value<std::string>()->default_value(port), "port this server will recieve messages on");

    return d;
}

po::variables_map parse_options(int argc, char* argv[], po::options_description& desc)
{
    po::variables_map v;
    po::store(po::parse_command_line(argc, argv, desc), v);
    po::notify(v);

    return v;
}

typedef std::map<std::string, std::string> port_map;

struct user_info
{
    std::string id;
    std::string ip;
    std::string port;
    std::string response_service_address;
    n::connection* sock;
};
typedef std::map<std::string, user_info> user_info_map;

void register_user(n::connection* sock, const ms::greet_register& r, user_info_map& m)
{
    CHECK(sock);
    if(r.id().empty()) return;

    //use user specified ip, otherwise use socket ip
    std::string ip = r.ip().empty() ? sock->socket().remote_endpoint().address().to_string() : r.ip();
    std::string port = r.ip().empty() ? boost::lexical_cast<std::string>(sock->socket().remote_endpoint().port()) : r.port();

    user_info i = {r.id(), ip, port, r.response_service_address(), sock};
    m[i.id] = i;

    std::cerr << "registered " << i.id << " " << i.ip << ":" << i.port << std::endl;
}

void send_response(const ms::greet_find_response& r, const user_info& u)
{
    CHECK(u.sock);
    m::message m = r;

    auto address = n::make_tcp_address(u.ip, u.port); 
    m.meta.to = {address, u.response_service_address};

    std::cerr << "sending reply to " << address << std::endl;
    u.sock->send(u::encode(m));
}

void find_user(const ms::greet_find_request& r, user_info_map& users)
{
    //find from user
    auto fup = users.find(r.from_id());
    if(fup == users.end()) return;

    //find search user
    auto up = users.find(r.search_id());
    if(up == users.end()) return;

    auto& f = fup->second;
    auto& i = up->second;

    std::cerr << "found match " << f.id << " " << f.ip << ":" << f.port << " <==> " <<  i.id << " " << i.ip << ":" << i.port << std::endl;

    //send response to both clients
    ms::greet_find_response fr{true, i.id, i.ip,  i.port};
    send_response(fr, f);

    ms::greet_find_response ir{true, f.id, f.ip,  f.port};
    send_response(ir, i);
}

int main(int argc, char *argv[])
{
    auto desc = create_descriptions();
    auto vm = parse_options(argc, argv, desc);
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    auto host = vm["host"].as<std::string>();
    auto port = vm["port"].as<std::string>();

    //it is import the connection manager is created before
    //the input connection is made. This is because tcp on
    //linux requires that binds to the same port are made before
    //a listen is made.
    n::connection_manager con{POOL_SIZE, port};
    user_info_map users;

    u::bytes data;
    while(true)
    try
    {
        if(!con.recieve(data))
        {
            u::sleep_thread(THREAD_SLEEP);
            continue;
        }

        //parse message
        m::message m;
        u::decode(data, m);

        //get socket info of the message just recieved.
        //because we are tracking incoming
        auto socket = con.get_socket();

        if(m.meta.type == ms::GREET_REGISTER)
        {
            ms::greet_register r{m};
            register_user(socket, r, users);
        }
        else if(m.meta.type == ms::GREET_FIND_REQUEST)
        {
            ms::greet_find_request r{m};
            find_user(r, users);
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "error parsing message: " << e.what() << std::endl;
        std::cerr << "message: " << u::to_str(data) << std::endl;
    }
    catch(...)
    {
        std::cerr << "unknown error parsing message: " << std::endl;
        std::cerr << "message: " << u::to_str(data) << std::endl;
    }
}
