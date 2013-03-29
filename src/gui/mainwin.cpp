/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#include "gui/mainwin.hpp"

#include "gui/app/chat_sample.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"

#include "gui/contactlist.hpp"
#include "gui/debugwin.hpp"
#include "gui/message.hpp"
#include "gui/session.hpp"
#include "gui/util.hpp"

#include "network/message_queue.hpp"

#include "message/master_post.hpp"
#include "messages/new_app.hpp"

#include "util/mencode.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <sstream>
#include <stdexcept>

#include <QtGui>
#include <QSignalMapper>

namespace m = fire::message;
namespace ms = fire::messages;
namespace u = fire::util;
namespace us = fire::user;
namespace s = fire::session;
namespace a = fire::gui::app;
namespace n = fire::network;

namespace fire
{
    namespace gui
    {

        namespace
        {
            const std::string NEW_APP_S = "<new app>";
            const std::string GUI_MAIL = "gui";
            const QString NEW_SESSION_NAME = "new"; 
            const size_t TIMER_SLEEP = 100; //in milliseconds
        }

        main_window::main_window(const main_window_context& c) :
            _start_screen{0},
            _start_screen_attached{false},
            _alert_screen{0},
            _alerts{0},
            _mail{0},
            _master{0},
            _about_action{0},
            _close_action{0},
            _create_session_action{0},
            _rename_session_action{0},
            _quit_session_action{0},
            _debug_window_action{0},
            _main_menu{0},
            _contact_menu{0},
            _app_menu{0},
            _debug_menu{0},
            _root{0},
            _layout{0},
            _sessions{0},
            _context(c)
        {
            setup_post();
            setup_services();
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

            auto user = std::make_shared<us::local_user>(name);
            us::save_user(home, *user);

            ENSURE(user);
            return user;
        }

        us::local_user_ptr setup_user(const std::string& home)
        {
            auto user = us::load_user(home);
            return user ? user : make_new_user(home);
        }

        void main_window::setup_post()
        {
            REQUIRE(!_master);

            //create post office to handle incoming and outgoing messages
            _master = std::make_shared<m::master_post_office>(_context.host, _context.port);

            //create mailbox just for gui specific messages.
            //This mailbox is not connected to a post as only internally accessible
            _mail = std::make_shared<m::mailbox>(GUI_MAIL);
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
            connect(_sessions, SIGNAL(currentChanged(int)), this, SLOT(tab_changed(int)));

            create_alert_screen();
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

        void main_window::create_alert_screen()
        {
            REQUIRE_FALSE(_alert_screen);
            REQUIRE_FALSE(_alerts);

            _alert_screen = new QWidget;
            auto l = new QVBoxLayout;
            _alert_screen->setLayout(l);
            _alerts = new list;
            l->addWidget(_alerts);
            _alert_screen->hide();

            ENSURE(_alert_screen);
            ENSURE(_alerts);
        }

        void main_window::create_menus()
        {
            REQUIRE_FALSE(_main_menu);
            REQUIRE_FALSE(_app_menu);
            REQUIRE_FALSE(_contact_menu);
            REQUIRE_FALSE(_debug_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);
            REQUIRE(_contact_list_action);
            REQUIRE(_chat_sample_action);
            REQUIRE(_app_editor_action);
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
            _session_menu->addAction(_quit_session_action);

            create_app_menu();

            if(_context.debug)
            {
                CHECK(_debug_window_action);
                _debug_menu = new QMenu{tr("&Debug"), this};
                _debug_menu->addAction(_debug_window_action);
            }

            menuBar()->addMenu(_main_menu);
            menuBar()->addMenu(_contact_menu);
            menuBar()->addMenu(_session_menu);
            menuBar()->addMenu(_app_menu);
            if(_debug_menu) menuBar()->addMenu(_debug_menu);

            ENSURE(_main_menu);
            ENSURE(_contact_menu);
            ENSURE(_app_menu);
        }

        void main_window::create_app_menu()
        {
            REQUIRE_FALSE(_app_menu);
            REQUIRE(_app_service);

            _app_menu = new QMenu{tr("&App"), this};

            update_app_menu();
        }

        void main_window::update_app_menu()
        {
            INVARIANT(_app_service);
            INVARIANT(_app_menu);

            _app_menu->clear();

            _app_menu->addAction(_chat_sample_action);
            _app_menu->addAction(_app_editor_action);

            for( auto p : _app_service->available_apps())
            {
                auto name = p.second.name;
                auto id = p.second.id;

                auto mapper = new QSignalMapper{this};
                auto action  = new QAction{name.c_str(), this};
                mapper->setMapping(action, QString(id.c_str()));
                connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(QString)), this, SLOT(load_app_into_session(QString)));

                _app_menu->addAction(action);
            }
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
            REQUIRE_FALSE(_quit_session_action);
            REQUIRE_FALSE(_debug_window_action);

            _about_action = new QAction{tr("&About"), this};
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction{tr("&Exit"), this};
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            _contact_list_action = new QAction{tr("&Contacts"), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _chat_sample_action = new QAction{tr("&Chat"), this};
            connect(_chat_sample_action, SIGNAL(triggered()), this, SLOT(make_chat_sample()));

            _app_editor_action = new QAction{tr("&App Editor"), this};
            connect(_app_editor_action, SIGNAL(triggered()), this, SLOT(make_app_editor()));

            _create_session_action = new QAction{tr("&Create"), this};
            connect(_create_session_action, SIGNAL(triggered()), this, SLOT(create_session()));

            _rename_session_action = new QAction{tr("&Rename"), this};
            connect(_rename_session_action, SIGNAL(triggered()), this, SLOT(rename_session()));

            _quit_session_action = new QAction{tr("&Quit"), this};
            connect(_quit_session_action, SIGNAL(triggered()), this, SLOT(quit_session()));

            if(_context.debug)
            {
                _debug_window_action = new QAction{tr("&Debug Window"), this};
                connect(_debug_window_action, SIGNAL(triggered()), this, SLOT(show_debug_window()));
            }

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_contact_list_action);
            ENSURE(_create_session_action);
            ENSURE(_rename_session_action);
            ENSURE(_quit_session_action);
            ENSURE(!_context.debug || _debug_window_action);
        }

        void main_window::setup_services()
        {
            REQUIRE_FALSE(_user_service);
            REQUIRE_FALSE(_session_service);
            REQUIRE_FALSE(_app_service);
            REQUIRE(_master);
            REQUIRE(_mail);

            us::user_service_context uc
            {
                _context.home,
                _context.host,
                _context.port,
                _mail,
            };

            _user_service = std::make_shared<us::user_service>(uc);
            _master->add(_user_service->mail());

            _session_service = std::make_shared<s::session_service>(_master, _user_service, _mail);
            _master->add(_session_service->mail());

            _app_service = std::make_shared<a::app_service>(_user_service, _mail);

            ENSURE(_user_service);
            ENSURE(_session_service);
            ENSURE(_app_service);
        }

        void main_window::show_contact_list()
        {
            ENSURE(_user_service);

            contact_list_dialog cl{"contacts", _user_service, false, this};
            cl.exec();
        }

        void main_window::show_contact_list_start()
        {
            ENSURE(_user_service);

            contact_list_dialog cl{"contacts", _user_service, true, this};
            cl.exec();
        }

        void main_window::show_debug_window()
        {
            REQUIRE(_context.debug);
            REQUIRE(_debug_menu);
            REQUIRE(_debug_window_action);
            REQUIRE(_master);
            REQUIRE(_user_service);
            REQUIRE(_session_service);

            auto db = new debug_win{_master,_user_service, _session_service};
            db->setAttribute(Qt::WA_DeleteOnClose);
            db->show();
            db->raise();
            db->activateWindow();
        }

        void main_window::make_chat_sample()
        {
            INVARIANT(_sessions);

            auto s = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!s) return;

            //create chat sample
            auto t = new a::chat_sample{s->session()};
            s->add(t);

            //add to master post so it can receive messages
            //from outside world
            _master->add(t->mail());

            //send new app message to contacts in session
            ms::new_app n{t->id(), t->type()}; 

            s->session()->send(n);
        }

        bool ask_user_to_select_app(QWidget* w, const a::app_service& apps, std::string& id)
        {
            QStringList as;
            as << NEW_APP_S.c_str();
            for(const auto& a : apps.available_apps())
                as << a.second.name.c_str();

            bool ok;
            auto g = QInputDialog::getItem(w, w->tr("Select App"),
                    w->tr("Choose an app:"), as, 0, false, &ok);

            if (!ok || g.isEmpty()) return false;

            std::string name = convert(g);
            if(name == NEW_APP_S) id = "";
            else 
                for(const auto& a : apps.available_apps())
                    if(a.second.name == name) { id = a.second.id; break; }

            return true;
        }

        void main_window::make_app_editor()
        {
            INVARIANT(_sessions);
            INVARIANT(_app_service);

            auto s = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!s) return;

            std::string id; 
            if(!ask_user_to_select_app(this, *_app_service, id)) return;

            a::app_ptr app;

            //if the app exists, load it
            if(!id.empty()) app = _app_service->load_app(id);

            //create app editor
            auto t = new a::app_editor{_app_service, s->session(), app};

            s->add(t);

            //add to master post so it can receive messages
            //from outside world
            _master->add(t->mail());

            //send new app message to contacts in session
            ms::new_app n{t->id(), t->type()}; 

            s->session()->send(n);
        }

        void main_window::load_app_into_session(QString qid)
        {
            INVARIANT(_app_service);
            INVARIANT(_sessions);

            //get current session
            auto s = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!s) return;

            //load app
            auto id = convert(qid);
            auto a = _app_service->load_app(id);
            if(!a) return;

            //create app widget
            auto t = new a::script_app{a, _app_service, s->session()};

            //add to session
            s->add(t);

            //add widget mailbox to master
            _master->add(t->mail());

            //send new app message to contacts in session
            m::message app_message = *a;
            ms::new_app n{t->id(), t->type(), u::encode(app_message)}; 

            s->session()->send(n);
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
        try
        {
            INVARIANT(_mail);

            m::message m;
            while(_mail->pop_inbox(m))
            try
            {
                if(m.meta.type == s::event::NEW_SESSION)
                {
                    s::event::new_session r;
                    s::event::convert(m, r);

                    new_session_event(r.session_id);
                }
                else if(m.meta.type == s::event::QUIT_SESSION)
                {
                    s::event::quit_session r;
                    s::event::convert(m, r);

                    quit_session_event(r.session_id);
                }
                else if(m.meta.type == s::event::SESSION_SYNCED)
                {
                    session_synced_event(m);
                }
                else if(m.meta.type == s::event::CONTACT_REMOVED)
                {
                    contact_removed_from_session_event(m);
                }
                else if(m.meta.type == us::event::NEW_CONTACT)
                {
                    us::event::new_contact r;
                    us::event::convert(m, r);

                    new_contact_event(r.id);
                }
                else if(m.meta.type == us::event::CONTACT_CONNECTED)
                {
                    us::event::contact_connected r;
                    us::event::convert(m, r);
                    contact_connected_event(r);
                }
                else if(m.meta.type == us::event::CONTACT_DISCONNECTED)
                {
                    us::event::contact_disconnected r;
                    us::event::convert(m, r);
                    contact_disconnected_event(r);
                }
                else if(m.meta.type == a::event::APPS_UPDATED)
                {
                    a::event::apps_updated r;
                    a::event::convert(m, r);
                    apps_updated_event(r);
                }
                else
                {
                    throw std::runtime_error(m.meta.type + " is an unknown message type.");
                }
            }
            catch(std::exception& e)
            {
                LOG << "Error recieving message in `" << _mail->address() << "'. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "Unexpected error recieving message in `" << _mail->address() << "'" << std::endl;
            }
        }
        catch(std::exception& e)
        {
            LOG << "main_window: error in check_mail. " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "main_window: unexpected error in check_mail." << std::endl;
        }

        void main_window::tab_changed(int i)
        {
            INVARIANT(_create_session_action);
            INVARIANT(_rename_session_action);
            INVARIANT(_quit_session_action);
            INVARIANT(_sessions);
            INVARIANT(_app_menu);

            session_widget* s = nullptr;

            if(i != -1) s = dynamic_cast<session_widget*>(_sessions->widget(i));

            bool enabled = s != nullptr;

            _rename_session_action->setEnabled(enabled);
            _quit_session_action->setEnabled(enabled);
            _app_menu->setEnabled(enabled);
        }
        
        void main_window::create_session()
        {
            REQUIRE(_session_service);
            _session_service->create_session();
        }

        void main_window::create_session(QString id)
        {
            REQUIRE(_session_service);

            auto sid = convert(id);

            auto c = _user_service->user().contacts().by_id(sid);
            if(!c) return;

            us::contact_list l;
            l.add(c);
            
            _session_service->create_session(l);
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

        void main_window::quit_session()
        {
            REQUIRE(_sessions);

            auto sw = dynamic_cast<session_widget*>(_sessions->currentWidget());
            if(!sw) return;

            auto s = sw->session();
            CHECK(s);

            std::stringstream msg;
            msg << "Are you sure you want to quit the session `" << convert(sw->name()) << "'?";
            auto a = QMessageBox::warning(this, tr("Quit Session?"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
            if(a != QMessageBox::Yes) return;

            _session_service->quit_session(s->id());
        }

        void main_window::attach_start_screen()
        {
            INVARIANT(_start_screen);
            INVARIANT(_sessions);

            if(_start_screen_attached) return;

            _sessions->addTab(_start_screen, "start");
            _sessions->show();
            _start_screen_attached = true;

            ENSURE(_start_screen_attached);
            ENSURE(_sessions->isVisible());
        }

        void main_window::show_alert(QWidget* a)
        {
            REQUIRE(a);
            INVARIANT(_alert_screen);

            if(!_alert_screen->isVisible())
            {
                attach_start_screen();
                CHECK(_sessions->isVisible());

                _alert_screen->show();
                _sessions->addTab(_alert_screen, "alert");
            }

            _alerts->add(a);

            ENSURE(_sessions->isVisible());
        }

        void main_window::new_session_event(const std::string& id)
        {
            INVARIANT(_app_service);
            INVARIANT(_session_service);
            INVARIANT(_sessions);

            attach_start_screen();

            auto s = _session_service->session_by_id(id);
            if(!s) return;

            auto sw = new session_widget{_session_service, s, _app_service};

            std::string name = convert(NEW_SESSION_NAME);

            //make default name to be firt person in contact list
            if(!s->contacts().empty()) 
                name = s->contacts().list()[0]->name();

            //create the sessions widget
            sw->name(name.c_str());
            _sessions->addTab(sw, name.c_str());

            ENSURE(_sessions->isVisible());
        }

        int find_session(QTabWidget* sessions, const std::string& id)
        {
            REQUIRE(sessions);

            //find correct tab
            int ri = -1;
            for(int i = 0; i < sessions->count(); i++)
            {
                auto sw = dynamic_cast<session_widget*>(sessions->widget(i));
                if(!sw) continue;

                auto s = sw->session();
                CHECK(s);
                if(s->id() == id)
                {
                    ri = i;
                    break;
                }
            }
            return ri;
        }

        void main_window::quit_session_event(const std::string& id)
        {
            INVARIANT(_app_service);
            INVARIANT(_session_service);
            INVARIANT(_sessions);

            //find correct tab
            int ri = find_session(_sessions, id);
            if(ri == -1) return;

            _sessions->removeTab(ri);
        }

        void main_window::session_synced_event(const m::message& m)
        {
            INVARIANT(_session_service);

            s::event::session_synced e;
            s::event::convert(m, e);

            auto s = _session_service->session_by_id(e.session_id);
            if(!s) return;

            CHECK(s->mail());
            s->mail()->push_inbox(m);
        }

        void main_window::contact_removed_from_session_event(const m::message& e)
        {
            _session_service->broadcast_message(e);
        }

        void main_window::new_contact_event(const std::string& id)
        {
            INVARIANT(_user_service);
            auto p = _user_service->pending_requests().find(id);
            if(p == _user_service->pending_requests().end()) return;

            auto c = p->second.from;
            CHECK(c);

            std::stringstream s;
            s << "<b>" << c->name() << "</b> wants to connect " << std::endl;

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            auto t = new QLabel{s.str().c_str()};
            auto b = new QPushButton{"contact list"};

            l->addWidget(t);
            l->addWidget(b);

            connect(b, SIGNAL(clicked()), this, SLOT(show_contact_list()));

            show_alert(w);
        }

        void main_window::contact_connected_event(const us::event::contact_connected& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_session_service);

            //get user
            auto c = _user_service->user().contacts().by_id(r.id);
            if(!c) return;

            //setup alert widget
            std::stringstream s;
            s << "<b>" << c->name() << "</b> is online" << std::endl;

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            auto t = new QLabel(s.str().c_str());
            l->addWidget(t);

            auto b = new QPushButton("new session");
            l->addWidget(b);
            auto m = new QSignalMapper(w);

            m->setMapping(b, QString{r.id.c_str()});
            connect(b, SIGNAL(clicked()), m, SLOT(map()));
            connect(m, SIGNAL(mapped(QString)), this, SLOT(create_session(QString)));

            //display alert
            show_alert(w);
            _session_service->broadcast_message(us::event::convert(r));
        }

        void main_window::contact_disconnected_event(const us::event::contact_disconnected& r)
        {
            INVARIANT(_user_service);

            //setup alert widget
            std::stringstream s;
            s << "<b>" << r.name << "</b> disconnected" << std::endl;

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);
            auto t = new QLabel{s.str().c_str()};
            l->addWidget(t);

            //display alert
            show_alert(w);
            _session_service->broadcast_message(us::event::convert(r));
        }

        void main_window::apps_updated_event(const app::event::apps_updated&)
        {
            update_app_menu();
        }
    }
}
