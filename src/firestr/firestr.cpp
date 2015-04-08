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

#include <QApplication>

#include "user/user.hpp"
#include "gui/setup.hpp"
#include "gui/main_win.hpp"
#include "gui/util.hpp"
#include "network/util.hpp"

#include "util/env.hpp"
#include "util/log.hpp"
#include "util/serialize.hpp"

#include <string>
#include <cstdlib>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;
namespace fg = fire::gui;
namespace fg = fire::gui;
namespace fn = fire::network;
namespace fu = fire::util;
namespace fus = fire::user;

namespace
{
    fn::port_type DEFAULT_PORT = 6060;
}

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    auto firestr_home = fu::get_default_firestr_home();

    std::string host = ip::host_name();
    std::string ip = fn::get_lan_ip();

    if(!ip.empty()) host = ip;

    d.add_options()
        ("help", "prints help")
        ("home", po::value<std::string>()->default_value(firestr_home), "configuration directory")
        ("host", po::value<std::string>()->default_value(host), "host/ip of this machine") 
        ("port", po::value<int>()->default_value(DEFAULT_PORT), "port this machine will receive messages on")
        ("debug", "if set, turns on the debug menu");

    return d;
}

po::variables_map parse_options(int argc, char* argv[], po::options_description& desc)
{
    po::variables_map v;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), v);
    po::notify(v);

    return v;
}


fn::port_type get_port(const std::string& home, fn::port_type cmd_port)
{
    if(cmd_port != DEFAULT_PORT) return cmd_port;
    auto cached_port = fus::load_port(home);
    return cached_port != 0 ? cached_port : cmd_port;
}

int main(int argc, char *argv[])
try
{
    auto desc = create_descriptions();
    auto vm = parse_options(argc, argv, desc);
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    fu::setup_env();

    QApplication a{argc, argv};

    fg::main_window_context c;
    c.home = vm["home"].as<std::string>();
    c.host = vm["host"].as<std::string>();
    c.port = get_port(c.home, vm["port"].as<int>());
    c.debug = vm.count("debug");

    CREATE_LOG(c.home);

    fg::setup_gui();

    c.user = fg::setup_user(c.home);
    if(!c.user) return 0;

    fus::save_port(c.home, c.port);

    fg::main_window w{c};
    w.show();

    auto rc = a.exec();
    LOG << "firestr shutting down..." << std::endl;
    return rc;
}
catch(std::exception& e)
{
    LOG << "program quit prematurely: " << e.what() << std::endl;
    return 1;
}
catch(...)
{
    LOG << "program quit prematurely: unknown reason" << std::endl;
    return 1;
}

#ifdef _WIN64
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
