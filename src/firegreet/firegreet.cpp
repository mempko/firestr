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

#include <string>
#include <cstdlib>
#include <fstream>
#include <termios.h>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "network/connection_manager.hpp"
#include "message/message.hpp"
#include "messages/greeter.hpp"
#include "security/security_library.hpp"
#include "util/thread.hpp"
#include "util/bytes.hpp"
#include "util/compress.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <botan/botan.h>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;
namespace bf = boost::filesystem;
namespace n = fire::network;
namespace m = fire::message;
namespace ms = fire::messages;
namespace u = fire::util;
namespace sc = fire::security;

namespace
{
    const size_t THREAD_SLEEP = 100; //in milliseconds
    const size_t POOL_SIZE = 10; //small pool size for now
}

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    const std::string host = ip::host_name();
    n::port_type port = 7070;
    const std::string private_key = "firegreet_key";

    d.add_options()
        ("help", "prints help")
        ("host", po::value<std::string>()->default_value(host), "host/ip of this server") 
        ("port", po::value<int>()->default_value(port), "port this server will receive messages on")
        ("pass", po::value<std::string>(), "password to decrypt private key")
        ("key", po::value<std::string>()->default_value(private_key), "path to private key file");

    return d;
}

po::variables_map parse_options(int argc, char* argv[], po::options_description& desc)
{
    po::variables_map v;
    po::store(po::parse_command_line(argc, argv, desc), v);
    po::notify(v);

    return v;
}

struct user_info
{
    std::string id;
    ms::greet_endpoint local;
    ms::greet_endpoint ext;
    std::string response_service_address;
    n::endpoint tcp_ep;
    n::endpoint udp_ep;
};
using user_info_map = std::map<std::string, user_info>;

void update_address(
        user_info& i, 
        const ms::greet_endpoint& local,
        const ms::greet_endpoint& ext,
        const n::endpoint& ep,
        bool is_udp)
{
    bool updated = false;

    if(i.local != local) { i.local = local; updated = true; }

    if(i.ext != ext)
    {
        if(is_udp) { i.ext = ext; updated = true; }
        else if(i.udp_ep.protocol.empty()) { i.ext = ext; updated = true; }
    }

    if(is_udp)
    {
        if(i.udp_ep != ep) { i.udp_ep = ep; updated = true; }
    }
    else 
    {
        if(i.tcp_ep != ep) { i.tcp_ep = ep; updated = true; }
    }

    if(updated)
    {
        LOG << "updated " << i.id << " " << i.ext.ip << ":" << i.ext.port << " prot: " << ep.protocol << std::endl;
    }

}

void register_user(
        n::connection_manager& con, 
        sc::encrypted_channels& sec, 
        const n::endpoint& ep, 
        const ms::greet_register& r, 
        user_info_map& m)
{
    auto address = n::make_address_str(ep);
    if(r.id().empty()) return;

    //use user specified ip, otherwise use socket ip
    ms::greet_endpoint local = r.local();
    ms::greet_endpoint ext = {ep.address, ep.port};

    //check to see if user already registers and
    //don't re-register if ip and port of local and external are the same
    bool is_udp = ep.protocol == "udp";
    if(m.count(r.id()))
    {
        auto& i = m[r.id()];
        update_address(i, local, ext, ep, is_udp);
    }
    else
    {
        n::endpoint tcp_ep;
        n::endpoint udp_ep;
        (is_udp ? udp_ep : tcp_ep) = ep;
        user_info i = {r.id(), local, ext, r.response_service_address(), tcp_ep, udp_ep};
        m[i.id] = i;
        LOG << "registered " << i.id << " " << i.ext.ip << ":" << i.ext.port << " prot: " << ep.protocol << std::endl;
    }

    sec.create_channel(address, r.pub_key());

}

void send_response(
        n::connection_manager& con, 
        sc::encrypted_channels& sec, 
        const ms::greet_find_response& r, 
        const user_info& u)
{
    REQUIRE_FALSE(u.tcp_ep.protocol.empty());
    m::message m = r;

    auto address = n::make_address_str(u.tcp_ep); 
    m.meta.to = {address, u.response_service_address};

    LOG << "sending reply to " << address << std::endl;

    //encrypt using public key
    auto data = u::encode(m);
    data = u::compress(data);
    data = sec.encrypt(address, data);
    con.send(address, data);
}

void find_user(
        n::connection_manager& con, 
        sc::encrypted_channels& sec, 
        const n::endpoint& ep, 
        const ms::greet_find_request& r, 
        user_info_map& users)
{
    //find from user
    auto fup = users.find(r.from_id());
    if(fup == users.end()) return;
    if(fup->second.tcp_ep.protocol.empty()) return;

    //find search user
    auto up = users.find(r.search_id());
    if(up == users.end()) return;
    if(up->second.tcp_ep.protocol.empty()) return;

    auto& f = fup->second;
    auto& i = up->second;

    //don't send match if either is disconnected
    if(con.is_disconnected(n::make_address_str(f.tcp_ep))) return;
    if(con.is_disconnected(n::make_address_str(i.tcp_ep))) return;

    LOG << "found match " << f.id << " " << f.ext.ip << ":" << f.ext.port << " <==> " <<  i.id << " " << i.ext.ip << ":" << i.ext.port << std::endl;

    //send response to both clients
    ms::greet_find_response fr{true, i.id, i.local,  i.ext};
    send_response(con, sec, fr, f);

    ms::greet_find_response ir{true, f.id, f.local,  f.ext};
    send_response(con, sec, ir, i);
}

void send_pub_key(
        n::connection_manager& con, 
        sc::encrypted_channels& sec, 
        const n::endpoint& ep,  
        const ms::greet_key_request& req, 
        user_info_map& users, 
        const sc::private_key& pkey)
{
    ms::greet_key_response rep{pkey.public_key()};
    m::message m = rep;

    auto address = n::make_address_str(ep); 
    m.meta.to = {address, req.response_service_address()};

    LOG << "sending pub key to " << address << std::endl;

    //send plaintext
    auto data = u::encode(m);
    data = u::compress(data);
    data = sec.encrypt(address, data);
    con.send(address, data);
}

void create_new_key(const std::string& key, const std::string& pass)
{
    auto k = std::make_shared<sc::private_key>(pass);
    CHECK(k);

    std::ofstream key_out(key.c_str(), std::fstream::out | std::fstream::binary);
    if(!key_out.good()) 
        throw std::runtime_error{"unable to save `" + key + "'"};

    sc::encode(key_out, *k);
}

sc::private_key_ptr load_private_key(const std::string& key, const std::string& pass)
{
    if(!bf::exists(key)) create_new_key(key, pass);

    std::ifstream key_in(key.c_str(), std::fstream::in | std::fstream::binary);
    if(!key_in.good()) 
        throw std::runtime_error{"unable to load `" + key + "' for reading"};

    auto r = sc::decode_private_key(key_in, pass);
    CHECK(r);
    return r;
}

void echo_off()
{
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void echo_on()
{
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string prompt_pass()
{
    std::string pass;
    std::cout << "enter password: ";
    echo_off();
    std::getline(std::cin, pass);
    echo_on();
    std::cout << std::endl;

    return pass;
}

int main(int argc, char *argv[])
{
    CREATE_LOG("./");

    auto desc = create_descriptions();
    auto vm = parse_options(argc, argv, desc);
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    Botan::LibraryInitializer init;

    auto host = vm["host"].as<std::string>();
    auto port = vm["port"].as<int>();
    auto pass = vm.count("pass") ? vm["pass"].as<std::string>() : prompt_pass();
    auto key = vm["key"].as<std::string>();

    auto pkey = load_private_key(key, pass);
    CHECK(pkey);

    n::connection_manager con{POOL_SIZE, static_cast<n::port_type>(port)};
    sc::encrypted_channels sec{*pkey};
    user_info_map users;

    u::bytes data;
    while(true)
    try
    {
        n::endpoint ep;
        if(!con.receive(ep, data))
        {
            u::sleep_thread(THREAD_SLEEP);
            continue;
        }

        //decrypt message
        auto sid = n::make_address_str(ep);
        sc::encryption_type et;
        data = sec.decrypt(sid, data, et);
        data = u::uncompress(data);

        //parse message
        m::message m;
        u::decode(data, m);

        if(m.meta.type == ms::GREET_REGISTER)
        {
            if(et != sc::encryption_type::asymmetric) continue;

            ms::greet_register r{m};
            register_user(con, sec, ep, r, users);
        }
        else if(m.meta.type == ms::GREET_FIND_REQUEST)
        {
            if(et != sc::encryption_type::asymmetric) continue;

            ms::greet_find_request r{m};
            find_user(con, sec,  ep, r, users);
        }
        else if(m.meta.type == ms::GREET_KEY_REQUEST)
        {
            ms::greet_key_request r{m};
            send_pub_key(con, sec, ep, r, users, *pkey);
        }
    }
    catch(std::exception& e)
    {
        LOG << "error parsing message: " << e.what() << std::endl;
    }
    catch(...)
    {
        LOG << "unknown error parsing message: " << std::endl;
    }
}
