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

#include <QApplication>

#include "gui/mainwin.hpp"

#include <string>
#include <cstdlib>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;
namespace fg = fire::gui;
namespace fn = fire::network;
namespace fu = fire::util;

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    std::string user = std::getenv("HOME");
    if(user.empty()) user = ".";
    const std::string home = user + "/.firestr";
    const std::string host = ip::host_name();
    const std::string port = "6060";
    const std::string ping_port = "6070";
    const std::string stun_server = "";
    const std::string stun_port = "3478";
    const std::string greeter_server = "";
    const std::string greeter_port = "7070";



    d.add_options()
        ("help", "prints help")
        ("home", po::value<std::string>()->default_value(home), "configuration directory")
        ("host", po::value<std::string>()->default_value(host), "host/ip of this machine") 
        ("port", po::value<std::string>()->default_value(port), "port this machine will recieve messages on")
        ("ping", po::value<std::string>()->default_value(ping_port), "port this machine will send pings on")
        ("stun-server", po::value<std::string>()->default_value(stun_server), "ip/host of stun server ")
        ("stun-port", po::value<std::string>()->default_value(stun_port), "port of the stun server")
        ("greeter-server", po::value<std::string>()->default_value(greeter_server), "ip/host of the greeter server")
        ("greeter-port", po::value<std::string>()->default_value(greeter_port), "port of the greeter server");

    return d;
}

po::variables_map parse_options(int argc, char* argv[], po::options_description& desc)
{
    po::variables_map v;
    po::store(po::parse_command_line(argc, argv, desc), v);
    po::notify(v);

    return v;
}

bool user_setup(const std::string& home)
{
    auto user = fg::setup_user(home);
    return user != nullptr;
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

    QApplication a{argc, argv};

    fg::main_window_context c;
        
    c.home = vm["home"].as<std::string>();
    c.host = vm["host"].as<std::string>();
    c.port = vm["port"].as<std::string>();
    c.ping_port = vm["ping"].as<std::string>();
    c.stun_server = vm["stun-server"].as<std::string>();
    c.stun_port = vm["stun-port"].as<std::string>();
    c.greeter_server = vm["greeter-server"].as<std::string>();
    c.greeter_port = vm["greeter-port"].as<std::string>();

    if(!user_setup(c.home)) return 0;

    fg::main_window w{c};

    w.show();

    return a.exec();
}
