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

#include "network/message_queue.hpp"
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

n::message_queue_ptr setup_input_connection(const std::string& port)
{
    auto address = n::make_bst_address("*", port);

    n::queue_options qo = { 
        {"bnd", "1"},
        {"block", "0"},
        {"track_incoming", "1"}};

    return n::create_message_queue(address, qo);
}

n::message_queue_ptr setup_output_connection(const std::string& address)
{
    n::queue_options qo = { 
        {"con", "1"},
        {"block", "0"}};
    return n::create_message_queue(address, qo);
}

struct user_info
{
    std::string id;
    std::string ip;
    std::string port;
    std::string return_port;
};
typedef std::map<std::string, user_info> user_info_map;

void register_user(const n::socket_info& sock, n::message_queue_ptr q, const ms::greet_register& r, user_info_map& m)
{
    if(r.id().empty()) return;

    //use user specified ip, otherwise use socket ip
    std::string ip = r.ip().empty() ? sock.remote_address : r.ip();

    user_info i = {r.id(), ip, r.port(), r.return_port()};
    m[i.id] = i;

    m::message rm = r;
    std::cerr << "registered " << i.id << " " << i.ip << ":" << i.port << std::endl;
}

void find_user(n::message_queue_ptr q, const ms::greet_find_request& r, user_info_map& users)
{
    CHECK(q);

    auto fup = users.find(r.from_id());
    auto up = users.find(r.search_id());

    if(fup == users.end()) return;

    const auto& f = fup->second;
    m::message m;

    if(up == users.end())
    {
        ms::greet_find_response rs{false, "", "", ""};
        m = rs;
    }
    else
    {
        const auto& i = up->second;
        std::cerr << "found " << i.id << " " << i.ip << ":" << i.port << std::endl;
        ms::greet_find_response rs{true, i.id, i.ip,  i.port};
        m = rs;
    }

    auto reply_to = n::make_bst_address(f.ip, f.port, f.return_port);
    m.meta.to = {reply_to, r.response_service_address()};
    
    std::cerr << "sending reply to " << reply_to << " " << r.response_service_address() << std::endl;
    auto rq = setup_output_connection(reply_to);
    CHECK(rq);
    rq->send(u::encode(m));
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

    auto in = setup_input_connection(port);
    user_info_map users;

    u::bytes data;
    while(true)
    try
    {
        if(!in->recieve(data))
        {
            u::sleep_thread(THREAD_SLEEP);
            continue;
        }

        //parse message
        m::message m;
        u::decode(data, m);

        //get socket info of the message just recieved.
        //because we are tracking incoming
        auto info = in->get_socket_info();

        if(m.meta.type == ms::GREET_REGISTER)
        {
            ms::greet_register r{m};
            register_user(info, in, r, users);
        }
        else if(m.meta.type == ms::GREET_FIND_REQUEST)
        {
            ms::greet_find_request r{m};
            find_user(in, r, users);
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
