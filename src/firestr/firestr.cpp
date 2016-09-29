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
#include <QStyleFactory>

#include "user/user.hpp"
#include "gui/setup.hpp"
#include "gui/main_win.hpp"
#include "gui/util.hpp"
#include "network/util.hpp"

#include "util/env.hpp"
#include "util/log.hpp"
#include "util/rand.hpp"
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

void assert_dialog(const char* msg) 
{ 
    fg::assert_dialog d{msg};
    d.exec();
}

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    auto firestr_home = fu::get_default_firestr_home();

    d.add_options()
        ("help", "prints help")
        ("home", po::value<std::string>()->default_value(firestr_home), "configuration directory")
        ("host", po::value<std::string>()->default_value(""), "host/ip of this machine") 
        ("port", po::value<int>()->default_value(DEFAULT_PORT), "port this machine will receive messages on. If not specified, then the port will be within 1000 of the default")
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

fn::port_type randomize_port(fn::port_type port)
{
    return port + fu::rand(0, 1000);
}

fn::port_type get_port(const std::string& home, fn::port_type cmd_port)
{
    if(cmd_port != DEFAULT_PORT) return cmd_port;
    auto cached_port = fus::load_port(home);
    return cached_port != 0 ? cached_port : randomize_port(cmd_port);
}

class firestr_app : public QApplication 
{
    public:
        firestr_app(int& argc, char ** argv) : QApplication{argc, argv} {}

        virtual ~firestr_app() {}
        virtual bool notify(QObject * receiver, QEvent* event) 
        try
        {
            return QApplication::notify(receiver, event);
        }
        catch(std::exception& e)
        {
            std::stringstream ss;
            ss << "There was an unexpected error : " << e.what() << std::endl;
            auto s = ss.str();
            LOG << s;
            fg::unexpected_error_dialog{s.c_str()}.exec();
            return false;
        }
        catch(...)
        {
            std::stringstream ss;
            ss << "There was an unexpected unknown error." << std::endl;
            auto s = ss.str();
            LOG << s;
            fg::unexpected_error_dialog{s.c_str()}.exec();
            return false;
        }
};

void set_fusion_theme(QApplication& app) 
{
    //make sure fusion is found
    bool has_fusion = false;
    for(auto k : QStyleFactory::keys()) 
        if(k == "Fusion") has_fusion = true;

    if(!has_fusion) return;

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(200,210,210));
    palette.setColor(QPalette::Base, QColor(230,240,240));
    palette.setColor(QPalette::AlternateBase, QColor(200,210,210));
    palette.setColor(QPalette::Button, QColor(200,210,210));

    palette.setColor(QPalette::Highlight, QColor(0,128,0));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);
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
    fu::set_assert_dialog_callback(assert_dialog);

    firestr_app a{argc, argv};
    set_fusion_theme(a);

    fg::main_window_context c;
    c.home = vm["home"].as<std::string>();
    c.host = vm["host"].as<std::string>();
    c.port = get_port(c.home, vm["port"].as<int>());
    c.debug = vm.count("debug");

    CREATE_LOG(c.home);

    fg::setup_gui();

    auto setup = fg::setup_user(c.home);

    c.user = setup.first;
    c.user_just_created = setup.second;

    if(!c.user) return 0;

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
