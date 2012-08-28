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
#include <sstream>
#include <stdexcept>

#include <QtGui>

#include "util/dbc.hpp"

#include "gui/mainwin.hpp"
#include "gui/message.hpp"
#include "gui/textmessage.hpp"
#include "util/mencode.hpp"
#include "network/message_queue.hpp"
#include "message/master_post.hpp"
#include "util/bytes.hpp"

namespace m = fire::message;
namespace u = fire::util;
                
namespace fire
{
    namespace gui
    {
        namespace m = fire::message;

        main_window::main_window(
                        const std::string& host, 
                        const std::string& port,
                        const std::string& home) :
            _about_action(0),
            _close_action(0),
            _main_menu(0),
            _root(0),
            _layout(0),
            _messages(0),
            _home(home)
        {
            setup_post(host, port);
            setup_user();
            create_actions();
            create_main();
            create_menus();

            INVARIANT(_main_menu);
            INVARIANT(_about_action);
            INVARIANT(_close_action);
        }

        void main_window::setup_user()
        {
            REQUIRE(_master);
            _user = user::load_user(_home);
            if(!_user)
            {
                user::user_info info(_master->address(), "nameless", "1234");
                user::users contacts;

                _user.reset(new user::local_user{info, contacts});
                user::save_user(_home, _user);
            }

            ENSURE(_user);
        }

        void main_window::setup_post(
                        const std::string& host, 
                        const std::string& port)
        {
            REQUIRE(!_master);

            _master.reset(new m::master_post_office{host, port});

            INVARIANT(_master);
        }

        void main_window::create_main()
        {
            REQUIRE_FALSE(_root);
            REQUIRE_FALSE(_layout);
            REQUIRE_FALSE(_messages);
            REQUIRE(_master);
            
            //setup main
            _root = new QWidget{this};
            _layout = new QVBoxLayout{_root};

            //setup message list
            _messages = new message_list{"main"};
            _layout->addWidget(_messages);
            _master->add(_messages->mail());

            //setup base
            setWindowTitle(tr("Firestr"));
            setCentralWidget(_root);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_messages);
        }

        void main_window::create_menus()
        {
            REQUIRE_FALSE(_main_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);

            _main_menu = new QMenu(tr("&Main"), this);
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            menuBar()->addMenu(_main_menu);

            ENSURE(_main_menu);
        }
        
        void main_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);

            _about_action = new QAction(tr("&About"), this);
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction(tr("&Exit"), this);
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            ENSURE(_about_action);
            ENSURE(_close_action);
        }

        void main_window::about()
        {
            QMessageBox::about(this, tr("About Firestr"),
                tr("<p><b>Firestr</b> allows you to communicate with people via multimedia "
                    "applications. Write, close, modify, and send people programs which "
                    "communicate with each other automatically, in a distributed way.</p>"
                    "<p>This is not the web, but it is on the internet.<br> "
                    "This is not a chat program, but a way for programs to chat.<br> "
                    "This is not a way to share code, but a way to share running software.</p> "
                    "<p>This program is created by <b>Maxim Noah Khailo</b> and is liscensed as GPLv3</p>"));
        }
    }
}
