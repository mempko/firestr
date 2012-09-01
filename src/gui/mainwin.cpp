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
#include "gui/contactlist.hpp"
#include "gui/message.hpp"
#include "gui/test_message.hpp"
#include "gui/util.hpp"
#include "util/mencode.hpp"
#include "network/message_queue.hpp"
#include "message/master_post.hpp"
#include "util/bytes.hpp"

namespace m = fire::message;
namespace u = fire::util;
namespace us = fire::user;
namespace s = fire::session;
                
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
            setup_services();
            create_actions();
            create_main();
            create_menus();

            INVARIANT(_master);
            INVARIANT(_main_menu);
            INVARIANT(_about_action);
            INVARIANT(_close_action);
        }

        us::local_user_ptr make_new_user(const std::string& home)
        {
            bool ok = false;
            std::string name = "me";

            auto r = QInputDialog::getText(
                    0, 
                    "Welcome!",
                    "Select User Name:",
                    QLineEdit::Normal, name.c_str(), &ok);

            if(ok && !r.isEmpty()) name = convert(r);
            else return {};

            us::local_user_ptr user{new us::local_user{name}};
            us::save_user(home, *user);

            ENSURE(user);
            return user;
        }

        us::local_user_ptr setup_user(const std::string& home)
        {
            auto user = us::load_user(home);
            return user ? user : make_new_user(home);
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
            REQUIRE(_user_service);
            REQUIRE(_session_service);
            
            //setup main
            _root = new QWidget{this};
            _layout = new QVBoxLayout{_root};

            //setup message list
            auto session = _session_service->create_session("test_session", _user_service->user().contacts());
            CHECK(session);
            _messages = new message_list{session};
            _layout->addWidget(_messages);

            std::string title = "Firestr - " + _user_service->user().info().name();
            //setup base
            setWindowTitle(tr(title.c_str()));
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
            REQUIRE(_contact_list_action);
            REQUIRE(_test_message_action);

            _main_menu = new QMenu{tr("&Main"), this};
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            _contact_menu = new QMenu{tr("&Contacts"), this};
            _contact_menu->addAction(_contact_list_action);

            _test_menu = new QMenu{tr("&Test"), this};
            _test_menu->addAction(_test_message_action);

            menuBar()->addMenu(_main_menu);
            menuBar()->addMenu(_contact_menu);
            menuBar()->addMenu(_test_menu);

            ENSURE(_main_menu);
            ENSURE(_contact_menu);
            ENSURE(_test_menu);
        }
        
        void main_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);

            _about_action = new QAction{tr("&About"), this};
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction{tr("&Exit"), this};
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            _contact_list_action = new QAction{tr("&Contacts"), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _test_message_action = new QAction{tr("&Test Message"), this};
            connect(_test_message_action, SIGNAL(triggered()), this, SLOT(make_test_message()));

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_contact_list_action);
        }

        void main_window::setup_services()
        {
            REQUIRE_FALSE(_user_service);
            REQUIRE(_master);

            _user_service.reset(new us::user_service{_home});
            _master->add(_user_service->mail());

            _session_service.reset(new s::session_service{_master, _user_service});
            _master->add(_session_service->mail());

            ENSURE(_user_service);
            ENSURE(_session_service);
        }

        void main_window::show_contact_list()
        {
            ENSURE(_user_service);

            contact_list cl{"contacts", _user_service};
            cl.exec();
        }

        void main_window::make_test_message()
        {
            INVARIANT(_messages);
            INVARIANT(_messages->session());

            auto* t = new test_message{_messages->session()->sender()};
            _messages->add(t);
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
