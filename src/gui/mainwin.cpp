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
#include "gui/session.hpp"
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

        namespace
        {
            const std::string GUI_MAIL = "gui";
            const QString NEW_SESSION_NAME = "new"; 
            const size_t TIMER_SLEEP = 100; //in milliseconds
        }

        main_window::main_window(
                const std::string& host, 
                const std::string& port,
                const std::string& ping,
                const std::string& home) :
            _start_screen(0),
            _mail(0),
            _master(0),
            _about_action(0),
            _close_action(0),
            _create_session_action(0),
            _rename_session_action(0),
            _main_menu(0),
            _root(0),
            _layout(0),
            _sessions(0),
            _home(home)
        {
            setup_post(host, port);
            setup_services(ping);
            create_actions();
            create_main();
            create_menus();
            restore_state();
            setup_timers();

            INVARIANT(_master);
            INVARIANT(_main_menu);
        }

        void main_window::save_state()
        {
            QSettings settings("mempko", "firestr");
            settings.setValue("main_window/geometry", saveGeometry());
            settings.setValue("main_window/state", saveState());
        }

        void main_window::restore_state()
        {
            QSettings settings("mempko", "firestr");
            restoreGeometry(settings.value("main_window/geometry").toByteArray());
            restoreState(settings.value("main_window/state").toByteArray());
        }

        void main_window::closeEvent(QCloseEvent *event)
        {
            save_state();
            QMainWindow::closeEvent(event);
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

            //create post office to handle incoming and outgoing messages
            _master.reset(new m::master_post_office{host, port});

            //create mailbox just for gui specific messages.
            //This mailbox is not connected to a post as only internally accessible
            _mail.reset(new m::mailbox{GUI_MAIL});
            INVARIANT(_master);
            INVARIANT(_mail);
        }

        void main_window::create_main()
        {
            REQUIRE_FALSE(_root);
            REQUIRE_FALSE(_layout);
            REQUIRE_FALSE(_sessions);
            REQUIRE(_master);
            REQUIRE(_user_service);
            REQUIRE(_session_service);

            //setup main
            _root = new QWidget{this};
            _layout = new QVBoxLayout{_root};

            //create the sessions widget
            _sessions = new QTabWidget;
            _layout->addWidget(_sessions);
            _sessions->hide();

            create_start_screen();
            _layout->addWidget(_start_screen, Qt::AlignCenter);

            std::string title = "Firestr - " + _user_service->user().info().name();
            //setup base
            setWindowTitle(tr(title.c_str()));
            setCentralWidget(_root);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_sessions);
            ENSURE(_start_screen);
        }

        void main_window::create_start_screen()
        {
            REQUIRE(_layout);
            REQUIRE(_user_service);
            REQUIRE_FALSE(_start_screen);

            _start_screen = new QWidget;
            auto l = new QVBoxLayout;
            _start_screen->setLayout(l);

            if(_user_service->user().contacts().empty())
            {
                auto intro = new QLabel(
                        "<b>Welcome!</b><br><br>"
                        "Add a new contact now.<br>"
                        "You need their <b>IP</b> and <b>PORT<b><br>"
                        "Once they accept, you are connected!"
                        );
                auto add_contact = new QPushButton("add contact");

                auto intro2 = new QLabel("Once connected, create a session");
                auto add_session = new QPushButton("create session");
                l->addWidget(intro);
                l->addWidget(add_contact);
                l->addWidget(intro2);
                l->addWidget(add_session);

                connect(add_contact, SIGNAL(clicked()), this, SLOT(show_contact_list_start()));
                connect(add_session, SIGNAL(clicked()), this, SLOT(create_session()));
            }
            else
            {
                auto intro = new QLabel(
                        "<b>Welcome!</b><br><br>"
                        "Start by creating a session"
                        );
                auto add_session = new QPushButton("create session");
                l->addWidget(intro);
                l->addWidget(add_session);
                connect(add_session, SIGNAL(clicked()), this, SLOT(create_session()));
            }
            l->setContentsMargins(20,10,20,10);

            ENSURE(_start_screen);
        }

        void main_window::create_menus()
        {
            REQUIRE_FALSE(_main_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);
            REQUIRE(_contact_list_action);
            REQUIRE(_test_message_action);
            REQUIRE(_create_session_action);

            _main_menu = new QMenu{tr("&Main"), this};
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            _contact_menu = new QMenu{tr("&Contacts"), this};
            _contact_menu->addAction(_contact_list_action);

            _session_menu = new QMenu{tr("&Session"), this};
            _session_menu->addAction(_create_session_action);
            _session_menu->addAction(_rename_session_action);

            _test_menu = new QMenu{tr("&Test"), this};
            _test_menu->addAction(_test_message_action);

            menuBar()->addMenu(_main_menu);
            menuBar()->addMenu(_contact_menu);
            menuBar()->addMenu(_session_menu);
            menuBar()->addMenu(_test_menu);

            ENSURE(_main_menu);
            ENSURE(_contact_menu);
            ENSURE(_test_menu);
        }

        void main_window::setup_timers()
        {
            //setup message timer
            //to get gui messages
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
            t->start(TIMER_SLEEP);
        }

        void main_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);
            REQUIRE_FALSE(_create_session_action);
            REQUIRE_FALSE(_rename_session_action);

            _about_action = new QAction{tr("&About"), this};
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction{tr("&Exit"), this};
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            _contact_list_action = new QAction{tr("&Contacts"), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _test_message_action = new QAction{tr("&Test Message"), this};
            connect(_test_message_action, SIGNAL(triggered()), this, SLOT(make_test_message()));

            _create_session_action = new QAction{tr("&Create"), this};
            connect(_create_session_action, SIGNAL(triggered()), this, SLOT(create_session()));

            _rename_session_action = new QAction{tr("&Rename"), this};
            connect(_rename_session_action, SIGNAL(triggered()), this, SLOT(rename_session()));

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_contact_list_action);
            ENSURE(_create_session_action);
            ENSURE(_rename_session_action);
        }

        void main_window::setup_services(const std::string& ping)
        {
            REQUIRE_FALSE(_user_service);
            REQUIRE(_master);
            REQUIRE(_mail);

            _user_service.reset(new us::user_service{_home, ping});
            _master->add(_user_service->mail());

            _session_service.reset(new s::session_service{_master, _user_service, _mail});
            _master->add(_session_service->mail());

            ENSURE(_user_service);
            ENSURE(_session_service);
        }

        void main_window::show_contact_list()
        {
            ENSURE(_user_service);

            contact_list_dialog cl{"contacts", _user_service};
            cl.exec();
        }

        void main_window::show_contact_list_start()
        {
            ENSURE(_user_service);

            contact_list_dialog cl{"contacts", _user_service, true};
            cl.exec();
        }

        void main_window::make_test_message()
        {
            INVARIANT(_sessions);

            auto s = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!s) return;

            auto* t = new test_message{s->session()};
            s->add(t);
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

        void main_window::check_mail()
        {
            INVARIANT(_mail);

            m::message m;
            while(_mail->pop_inbox(m))
            try
            {
                if(m.meta.type == s::NEW_SESSION_EVENT)
                {
                    s::new_session_event r;
                    convert(m, r);

                    new_session(r.session_id);
                }
                else
                {
                    throw std::runtime_error(m.meta.type + " is an unknown message type.");
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "Error recieving message in `" << _mail->address() << "'. " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "Unexpected error recieving message in `" << _mail->address() << "'" << std::endl;
            }
        }
        
        void main_window::create_session()
        {
            REQUIRE(_session_service);
            _session_service->create_session();
        }

        void main_window::rename_session()
        {
            REQUIRE(_sessions);

            auto s = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!s) return;

            bool ok = false;
            QString name = s->name().isEmpty() ? NEW_SESSION_NAME : s->name();

            name = QInputDialog::getText(
                    0, 
                    "Rename Session",
                    "Name",
                    QLineEdit::Normal, name, &ok);

            if(!ok || name.isEmpty()) return;

            s->name(name);
            _sessions->setTabText(_sessions->currentIndex(), name);
        }

        void main_window::new_session(const std::string& id)
        {
            REQUIRE(_session_service);
            REQUIRE(_sessions);

            if(_start_screen->isVisible())
            {
                _start_screen->hide();
                _sessions->show();
            }

            auto s = _session_service->session_by_id(id);
            if(!s) return;

            auto sw = new session_widget{_session_service, s};

            //create the sessions widget
            _sessions->addTab(sw, NEW_SESSION_NAME);

            ENSURE(_sessions->isVisible());
        }
    }
}
