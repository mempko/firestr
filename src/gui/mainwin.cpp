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
#include "gui/mainwin.hpp"

#include "gui/app/chat.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"


#include "gui/contactlist.hpp"
#include "gui/conversation.hpp"
#include "gui/debugwin.hpp"
#include "gui/message.hpp"
#include "gui/util.hpp"
#include "gui/icon.hpp"

#include "network/message_queue.hpp"

#include "message/master_post.hpp"
#include "messages/new_app.hpp"

#include "util/bytes.hpp"
#include "util/dbc.hpp"
#include "util/env.hpp"
#include "util/log.hpp"
#include "util/mencode.hpp"
#include "util/time.hpp"

#include <sstream>
#include <stdexcept>

#include <QtWidgets>
#include <QSignalMapper>

namespace m = fire::message;
namespace ms = fire::messages;
namespace u = fire::util;
namespace us = fire::user;
namespace s = fire::conversation;
namespace a = fire::gui::app;
namespace n = fire::network;
namespace sc = fire::security;

namespace fire
{
    namespace gui
    {

        namespace
        {
            const std::string NEW_APP_S = "<new app>";
            const std::string GUI_MAIL = "gui";
            const QString NEW_CONVERSATION_NAME = "new"; 
            const int MAIN_X = 80;
            const int MAIN_Y = 80;
            const int MAIN_W = 480;
            const int MAIN_H = 640;
        }

        main_window::main_window(const main_window_context& c) :
            _context(c)
        {
            setWindowIcon(logo());
            setAcceptDrops(true);
            setGeometry(MAIN_X, MAIN_Y, MAIN_W, MAIN_H);
            init_handlers();
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

        main_window::~main_window()
        {
            INVARIANT(_mail_service);
            _mail_service->done();
        }

        void main_window::save_state()
        {
            INVARIANT(_context.user);
            auto id = app_id(*_context.user);
            QSettings settings("mempko", id.c_str());
            settings.setValue("main_window/geometry", saveGeometry());
            settings.setValue("main_window/state", saveState());
        }

        void main_window::restore_state()
        {
            INVARIANT(_context.user);
            auto id = app_id(*_context.user);

            QSettings settings("mempko", id.c_str());
            restoreGeometry(settings.value("main_window/geometry").toByteArray());
            restoreState(settings.value("main_window/state").toByteArray());
        }

        void main_window::closeEvent(QCloseEvent *event)
        {
            save_state();
            QMainWindow::closeEvent(event);
        }

        bool has_finvite_or_app(const QList<QUrl>& urls)
        {
            for(const auto& url : urls)
                if(url.toLocalFile().endsWith(".finvite") ||
                   url.toLocalFile().endsWith(".fab"))
                    return true;
            return false;
        }

        void main_window::dragEnterEvent(QDragEnterEvent* e)
        {
            REQUIRE(e);
            auto md = e->mimeData();
            if(!md) return;
            if(has_finvite_or_app(md->urls())) e->acceptProposedAction();
        }

        void main_window::dropEvent(QDropEvent* e)
        {
            REQUIRE(e);
            const auto* md = e->mimeData();
            if(!md) return;
            if(!md->hasUrls()) return;

            auto urls = md->urls();
            for(const auto& url : urls)
            {
                auto local = url.toLocalFile();
                if(local.endsWith(".finvite"))
                {
#ifdef _WIN64
                    auto cf = convert16(local);
#else
                    auto cf = convert(local);
#endif
                    add_contact_gui(_user_service, cf, this);
                }
                else if(local.endsWith(".fab"))
                {
                    install_app(convert(local));
                }

            }
        }

        us::local_user_ptr load_user(const std::string& home)
        {
            //loop if wrong password
            //load user throws error if the pass is wrong
            bool error = true;
            while(error)
            try
            {
                bool ok = false;
                std::string pass = "";

                auto p = QInputDialog::getText(
                        0, 
                        qApp->tr("Enter Password"),
                        qApp->tr("Password"),
                        QLineEdit::Password, pass.c_str(), &ok);

                if(ok && !p.isEmpty()) pass = convert(p);
                else return {};

                auto user = us::load_user(home, pass);
                error = false;
                return user;
            }
            catch(std::exception& e) 
            {
                LOG << "Error loading user: " << e.what() << std::endl;
            }

            return {};
        }

        us::local_user_ptr make_new_user(const std::string& home)
        {
            bool ok = false;
            std::string name = "your name here";

            auto r = QInputDialog::getText(
                    0, 
                    qApp->tr("New User"),
                    qApp->tr("Select User Name"),
                    QLineEdit::Normal, name.c_str(), &ok);

            if(ok && !r.isEmpty()) name = convert(r);
            else return {};

            std::string pass = "";
            auto p = QInputDialog::getText(
                    0, 
                    qApp->tr("Create Password"),
                    qApp->tr("Password"),
                    QLineEdit::Password, pass.c_str(), &ok);

            if(ok && !p.isEmpty()) pass = convert(p);
            else return {};

            auto key = std::make_shared<sc::private_key>(pass);
            auto user = std::make_shared<us::local_user>(name, key);
            us::save_user(home, *user);

            ENSURE(user);
            return user;
        }

        us::local_user_ptr setup_user(const std::string& home)
        {
            return us::user_created(home) ? load_user(home) : make_new_user(home);
        }

        void main_window::setup_post()
        {
            REQUIRE(!_master);

            //setup the encrypted channels which will handle security conversations
            //with contacts
            _encrypted_channels = std::make_shared<sc::encrypted_channels>(_context.user->private_key());

            //create post office to handle incoming and outgoing messages
            _master = std::make_shared<m::master_post_office>(
                    _context.host, 
                    _context.port, 
                    _encrypted_channels);

            //create mailbox just for gui specific messages.
            //This mailbox is not connected to a post and is only internally accessible
            _mail = std::make_shared<m::mailbox>(GUI_MAIL);
            INVARIANT(_master);
            INVARIANT(_mail);
            INVARIANT(_encrypted_channels);
        }

        void main_window::create_main()
        {
            REQUIRE_FALSE(_root);
            REQUIRE_FALSE(_layout);
            REQUIRE_FALSE(_conversations);
            REQUIRE(_master);
            REQUIRE(_user_service);
            REQUIRE(_conversation_service);

            //setup main
            _root = new QWidget{this};
            _layout = new QVBoxLayout{_root};

            //create the conversations widget
            _conversations = new MainTabs;
            _layout->addWidget(_conversations);
            _conversations->hide();
            connect(_conversations, SIGNAL(currentChanged(int)), this, SLOT(tab_changed(int)));
            connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(focus_changed(QWidget*,QWidget*)));

            create_alert_screen();
            create_start_screen();
            _layout->addWidget(_start_screen, Qt::AlignCenter);

            std::string title = "Fire★ - " + _user_service->user().info().name();
            //setup base
            setWindowTitle(tr(title.c_str()));
            setCentralWidget(_root);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_conversations);
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
                        tr(
                        "<b>Welcome!</b><br><br>"
                        "Add a new contact now.<br>"
                        "You need to create an invite file,<br>"
                        "and give it to another.<br>"
                        "Once you both add each other,<br>"
                        "you are connected!"
                        ));
                auto ac = new QPushButton(tr("connect with someone"));

                auto intro2 = new QLabel(tr("Once connected, start a conversation"));
                auto add_conversation = new QPushButton(tr("start conversation"));
                l->addWidget(intro);
                l->addWidget(ac);
                l->addWidget(intro2);
                l->addWidget(add_conversation);

                connect(ac, SIGNAL(clicked()), this, SLOT(show_contact_list()));
                connect(add_conversation, SIGNAL(clicked()), this, SLOT(create_conversation()));
            }
            else
            {
                _start_contacts = new contact_list{_user_service, _user_service->user().contacts(), 
                    [&](us::user_info_ptr u) {
                        REQUIRE(u);

                        auto con = new QPushButton;
                        std::stringstream ss;
                        ss << "New converesation with `" << u->name() << "'";
                        con->setToolTip(ss.str().c_str());
                        make_new_conversation_small(*con);

                        auto mapper = new QSignalMapper{this};
                        mapper->setMapping(con, QString(u->id().c_str()));
                        connect(con, SIGNAL(clicked()), mapper, SLOT(map()));
                        connect(mapper, SIGNAL(mapped(QString)), this, SLOT(create_conversation(QString)));

                        return new user_info{u, _user_service, true, con};
                    }
                };
                _start_contacts->update_status(true);

                auto add_conversation = new QPushButton;
                make_new_conversation(*add_conversation);
                add_conversation->setToolTip(tr("Create a new conversation"));
                l->addWidget(_start_contacts);
                l->addWidget(add_conversation);
                connect(add_conversation, SIGNAL(clicked()), this, SLOT(create_conversation()));
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
            REQUIRE(_create_invite_action);
            REQUIRE(_add_contact_action);
            REQUIRE(_contact_list_action);
            REQUIRE(_chat_app_action);
            REQUIRE(_app_editor_action);
            REQUIRE(_create_conversation_action);

            _main_menu = new QMenu{tr("&Main"), this};
            _main_menu->addAction(_about_action);
            _main_menu->addAction(_install_app_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            _contact_menu = new QMenu{tr("&Contacts"), this};
            _contact_menu->addAction(_create_invite_action);
            _contact_menu->addAction(_add_contact_action);
            _contact_menu->addAction(_contact_list_action);

            _conversation_menu = new QMenu{tr("C&onversation"), this};
            _conversation_menu->addAction(_create_conversation_action);
            _conversation_menu->addAction(_rename_conversation_action);
            _conversation_menu->addAction(_quit_conversation_action);

            create_app_menu();

            if(_context.debug)
            {
                CHECK(_debug_window_action);
                _debug_menu = new QMenu{tr("&Debug"), this};
                _debug_menu->addAction(_debug_window_action);
            }

            menuBar()->addMenu(_main_menu);
            menuBar()->addMenu(_contact_menu);
            menuBar()->addMenu(_conversation_menu);
            menuBar()->addMenu(_app_menu);
            if(_debug_menu) menuBar()->addMenu(_debug_menu);
            menuBar()->setHidden(false);

            tab_changed(-1);

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

            _app_menu->addAction(_chat_app_action);
            _app_menu->addAction(_app_editor_action);

            for( auto p : _app_service->available_apps())
            {
                auto name = p.second.name;
                auto id = p.second.id;

                auto mapper = new QSignalMapper{this};
                auto action  = new QAction{name.c_str(), this};
                mapper->setMapping(action, QString(id.c_str()));
                connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(QString)), this, SLOT(load_app_into_conversation(QString)));

                _app_menu->addAction(action);
            }
        }

        void main_window::setup_timers()
        {
            INVARIANT(_mail);
            _mail_service = new mail_service{_mail, this};
            _mail_service->start();
            ENSURE(_mail_service);
        }

        void main_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);
            REQUIRE_FALSE(_create_conversation_action);
            REQUIRE_FALSE(_rename_conversation_action);
            REQUIRE_FALSE(_quit_conversation_action);
            REQUIRE_FALSE(_debug_window_action);

            _about_action = new QAction{tr("&About"), this};
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction{tr("&Exit"), this};
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            _create_invite_action = new QAction{tr("&Save Invite"), this};
            connect(_create_invite_action, SIGNAL(triggered()), this, SLOT(create_invite()));

            _add_contact_action = new QAction{tr("&Add Contact"), this};
            connect(_add_contact_action, SIGNAL(triggered()), this, SLOT(add_contact()));

            _contact_list_action = new QAction{tr("&Contacts..."), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _chat_app_action = new QAction{tr("&Chat"), this};
            connect(_chat_app_action, SIGNAL(triggered()), this, SLOT(make_chat_app()));

            _app_editor_action = new QAction{tr("&App Editor"), this};
            connect(_app_editor_action, SIGNAL(triggered()), this, SLOT(make_app_editor()));

            _install_app_action = new QAction{tr("&Install App"), this};
            connect(_install_app_action, SIGNAL(triggered()), this, SLOT(install_app()));

            _create_conversation_action = new QAction{tr("&Create"), this};
            connect(_create_conversation_action, SIGNAL(triggered()), this, SLOT(create_conversation()));

            _rename_conversation_action = new QAction{tr("&Rename"), this};
            connect(_rename_conversation_action, SIGNAL(triggered()), this, SLOT(rename_conversation()));

            _quit_conversation_action = new QAction{tr("&Close"), this};
            connect(_quit_conversation_action, SIGNAL(triggered()), this, SLOT(quit_conversation()));

            if(_context.debug)
            {
                _debug_window_action = new QAction{tr("&Debug Window"), this};
                connect(_debug_window_action, SIGNAL(triggered()), this, SLOT(show_debug_window()));
            }

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_create_invite_action);
            ENSURE(_contact_list_action);
            ENSURE(_create_conversation_action);
            ENSURE(_rename_conversation_action);
            ENSURE(_quit_conversation_action);
            ENSURE(_chat_app_action);
            ENSURE(_app_editor_action);
            ENSURE(_install_app_action);
            ENSURE(!_context.debug || _debug_window_action);
        }

        void main_window::setup_services()
        {
            REQUIRE_FALSE(_user_service);
            REQUIRE_FALSE(_conversation_service);
            REQUIRE_FALSE(_app_service);
            REQUIRE(_master);
            REQUIRE(_mail);
            REQUIRE(_context.user);
            REQUIRE(_encrypted_channels);

            us::user_service_context uc
            {
                _context.home,
                _context.host,
                _context.port,
                _context.user,
                _mail,
                _encrypted_channels,
            };

            _user_service = std::make_shared<us::user_service>(uc);
            _master->add(_user_service->mail());

            _conversation_service = std::make_shared<s::conversation_service>(_master, _user_service, _mail);
            _master->add(_conversation_service->mail());

            _app_service = std::make_shared<a::app_service>(_user_service, _mail);

            ENSURE(_user_service);
            ENSURE(_conversation_service);
            ENSURE(_encrypted_channels);
            ENSURE(_app_service);
        }

        void main_window::create_invite()
        {
            INVARIANT(_user_service);
            create_contact_file(_user_service, this);
        }

        void main_window::add_contact()
        {
            INVARIANT(_user_service);
            gui::add_contact_gui(_user_service, this);
        }

        void main_window::show_contact_list()
        {
            INVARIANT(_user_service);

            contact_list_dialog cl{"contacts", _user_service, this};
            cl.exec();
            cl.save_state();
        }

        void main_window::show_debug_window()
        {
            REQUIRE(_context.debug);
            REQUIRE(_debug_menu);
            REQUIRE(_debug_window_action);
            REQUIRE(_master);
            REQUIRE(_user_service);
            REQUIRE(_conversation_service);

            auto db = new debug_win{
                _master,
                _user_service, 
                _conversation_service, 
                dynamic_cast<m::master_post_office*>(_master.get())->get_udp_stats()};
            db->setAttribute(Qt::WA_DeleteOnClose);
            db->show();
            db->raise();
            db->activateWindow();
        }

        void main_window::make_chat_app()
        {
            INVARIANT(_conversations);
            INVARIANT(_conversation_service);

            auto s = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!s) return;

            s->add_chat_app();
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
            INVARIANT(_conversations);
            INVARIANT(_app_service);
            INVARIANT(_conversation_service);

            auto s = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!s) return;

            std::string id; 
            if(!ask_user_to_select_app(this, *_app_service, id)) return;

            s->add_app_editor(id);
        }

        void main_window::install_app(const std::string& file)
        {
            REQUIRE_FALSE(file.empty());

            auto app = _app_service->import_app(file);
            CHECK(app);

            bool installed = install_app_gui(*app, *_app_service, this);

            if(installed)
            {
                std::stringstream ss;
                ss << "`" << app->name() << "' has been installed";
                QMessageBox::information(this, tr("App installed"), ss.str().c_str());
            }
        }

        void main_window::install_app()
        {
            INVARIANT(_app_service);

            auto home = u::get_home_dir();
            auto file = QFileDialog::getOpenFileName(this,
                    tr("Install App"), home.c_str(), tr("App (*.fab)"));

            if(file.isEmpty()) return;
            install_app(gui::convert(file));
        }

        void main_window::load_app_into_conversation(QString qid)
        {
            INVARIANT(_app_service);
            INVARIANT(_conversations);
            INVARIANT(_conversation_service);

            //get current conversation
            auto s = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!s) return;

            //load app
            std::string id = convert(qid);
            s->add_script_app(id);
        }

        void main_window::about()
        {
            QMessageBox::about(this, tr("Firestr 0.4"),
                    tr("<p><b>Fire★</b> is a simple distributed communication and computation "
                        "platform. Write, clone, modify, and send people programs which "
                        "communicate with each other automatically, in a distributed way.</p>"
                        "<p>This is not the web, but it is on the internet.<br> "
                        "This is not a chat program, but a way for programs to chat.<br> "
                        "This is not just a way to share code, but a way to share running software.</p> "
                        "<p>This program is created by <b>Maxim Noah Khailo</b> and is licensed as GPLv3</p>"));
        }

        void main_window::init_handlers()
        {
            using std::bind;
            using namespace std::placeholders;

            _service_map.handle(s::event::NEW_CONVERSATION,
                    bind(&main_window::received_new_conversation, this, _1));
            _service_map.handle(s::event::QUIT_CONVERSATION,
                    bind(&main_window::received_quit_conversation, this, _1));
            _service_map.handle(s::event::CONVERSATION_SYNCED,
                    bind(&main_window::received_conversation_synced, this, _1));
            _service_map.handle(s::event::CONTACT_REMOVED,
                    bind(&main_window::received_contact_removed_or_added_from_conversation, this, _1));
            _service_map.handle(s::event::CONTACT_ADDED,
                    bind(&main_window::contact_removed_or_added_from_conversation_event, this, _1));
            _service_map.handle(s::event::CONVERSATION_ALERT,
                    bind(&main_window::received_conversation_alert, this, _1));
            _service_map.handle(us::event::CONTACT_CONNECTED,
                    bind(&main_window::received_contact_connected, this, _1));
            _service_map.handle(us::event::CONTACT_DISCONNECTED,
                    bind(&main_window::received_contact_disconnected, this, _1));
            _service_map.handle(us::event::NEW_INTRODUCTION,
                    bind(&main_window::received_new_introduction, this, _1));
            _service_map.handle(a::event::APPS_UPDATED,
                    bind(&main_window::received_apps_updated, this, _1));
            _service_map.handle(s::event::NOT_PART_OF_CLIQUE,
                    bind(&main_window::received_not_part_of_clique, this, _1));

        }

        void main_window::received_new_conversation(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::NEW_CONVERSATION);

            s::event::new_conversation r;
            r.from_message(m);

            new_conversation_event(r.conversation_id);
        }

        void main_window::received_quit_conversation(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::QUIT_CONVERSATION);

            s::event::quit_conversation r;
            r.from_message(m);

            quit_conversation_event(r.conversation_id);
        }

        void main_window::received_conversation_synced(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::CONVERSATION_SYNCED);

            s::event::conversation_synced e;
            e.from_message(m);

            INVARIANT(_conversation_service);

            auto s = _conversation_service->conversation_by_id(e.conversation_id);
            if(!s) return;

            CHECK(s->mail());
            s->mail()->push_inbox(m);
        }

        void main_window::received_contact_removed_or_added_from_conversation(const m::message& m)
        {
            REQUIRE(m.meta.type == s::event::CONTACT_REMOVED || m.meta.type == s::event::CONTACT_ADDED);

            _conversation_service->broadcast_message(m);
        }

        void main_window::received_conversation_alert(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::CONVERSATION_ALERT);

            s::event::conversation_alert e;
            e.from_message(m);

            conversation_alert_event(e);
        }

        void main_window::received_contact_connected(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_CONNECTED);

            us::event::contact_connected r;
            r.from_message(m);

            contact_connected_event(r);
        }

        void main_window::received_contact_disconnected(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_DISCONNECTED);

            us::event::contact_disconnected r;
            r.from_message(m);

            contact_disconnected_event(r);
        }

        void main_window::received_new_introduction(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::NEW_INTRODUCTION);

            us::event::new_introduction n;
            n.from_message(m);

            new_intro_event(n);
        }

        void main_window::received_apps_updated(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, a::event::APPS_UPDATED);

            a::event::apps_updated r;
            r.from_message(m);

            apps_updated_event(r);
        }
        
        void main_window::received_not_part_of_clique(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::NOT_PART_OF_CLIQUE);

            s::event::not_part_of_clique r;
            r.from_message(m);
            not_part_of_clique_event(r);
        }

        void main_window::check_mail(m::message m)
        try
        {
            INVARIANT(_mail);

            if(!_service_map.handle(m))
                LOG << m.meta.type << " is an unknown message type in main window.";
        }
        catch(std::exception& e)
        {
            LOG << "Error recieving message in `" << _mail->address() << "'. " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "Unexpected error recieving message in `" << _mail->address() << "'" << std::endl;
        }

        void main_window::tab_changed(int i)
        {
            INVARIANT(_create_conversation_action);
            INVARIANT(_rename_conversation_action);
            INVARIANT(_quit_conversation_action);
            INVARIANT(_conversations);
            INVARIANT(_app_menu);
            INVARIANT(_conversations);

            _alert_tab_index = _conversations->indexOf(_alert_screen);

            conversation_widget* s = nullptr;

            auto sw = _conversations->widget(i);
            if(i != -1 && sw != nullptr) s = dynamic_cast<conversation_widget*>(sw);

            bool enabled = s != nullptr;

            _rename_conversation_action->setEnabled(enabled);
            _quit_conversation_action->setEnabled(enabled);
            _app_menu->setEnabled(enabled);

            if(i != -1 && (i == _alert_tab_index || enabled))
                _conversations->setTabTextColor(i, QColor{"black"});
        }
        
        void main_window::create_conversation()
        {
            REQUIRE(_conversation_service);
            _conversation_service->create_conversation();
        }

        void main_window::create_conversation(QString id)
        {
            REQUIRE(_conversation_service);

            auto sid = convert(id);

            auto c = _user_service->user().contacts().by_id(sid);
            if(!c) return;

            us::contact_list l;
            l.add(c);
            
            _conversation_service->create_conversation(l);
        }

        void main_window::rename_conversation()
        {
            REQUIRE(_conversations);

            auto s = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!s) return;

            bool ok = false;
            QString name = s->name().isEmpty() ? NEW_CONVERSATION_NAME : s->name();

            name = QInputDialog::getText(
                    0, 
                    tr("Rename Conversation"),
                    tr("Name"),
                    QLineEdit::Normal, name, &ok);

            if(!ok || name.isEmpty()) return;

            s->name(name);
            _conversations->setTabText(_conversations->currentIndex(), name);
        }

        void main_window::quit_conversation()
        {
            REQUIRE(_conversations);

            auto sw = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!sw) return;

            auto s = sw->conversation();
            CHECK(s);

            std::stringstream msg;
            msg << "Are you sure you want to close the conversation `" << convert(sw->name()) << "'?";
            auto a = QMessageBox::warning(this, tr("Close Conversation?"), tr(msg.str().c_str()), QMessageBox::Yes | QMessageBox::No);
            if(a != QMessageBox::Yes) return;

            _conversation_service->quit_conversation(s->id());
        }

        void main_window::attach_start_screen()
        {
            INVARIANT(_start_screen);
            INVARIANT(_conversations);

            if(_start_screen_attached) return;

            _conversations->addTab(_start_screen, "start");
            _conversations->show();
            _start_screen_attached = true;

            ENSURE(_start_screen_attached);
            ENSURE(_conversations->isVisible());
        }

        std::string formatted_timestamp()
        {
            std::stringstream s;
            s << "<font color='green'>" << u::timestamp() << "</font>";
            return s.str();
        }

        bool main_window::should_alert(int tab_index)
        {
            INVARIANT(_conversations);
            auto ct = _conversations->currentIndex();
            return (tab_index != -1 && tab_index != ct) || !_focus;
        }

        void main_window::alert_tab(int tab_index)
        {
            INVARIANT(_conversations);
            _conversations->setTabTextColor(tab_index, QColor{"red"});
            QApplication::alert(this);
        }

        void main_window::show_alert(QWidget* a)
        {
            REQUIRE(a);
            INVARIANT(_alert_screen);
            INVARIANT(_alerts);

            if(!_alert_screen->isVisible())
            {
                attach_start_screen();
                CHECK(_conversations->isVisible());

                _alert_screen->show();
                _alert_tab_index = _conversations->addTab(_alert_screen, tr("alert"));
            }

            CHECK_RANGE(_alert_tab_index, 0, _conversations->count());

            //create timestamp and put it to left of widget 
            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            auto t = new QLabel{formatted_timestamp().c_str()};
            auto x = make_x_button();

            l->addWidget(t);
            l->addWidget(a);
            l->addWidget(x);

            auto mapper = new QSignalMapper{this};
            mapper->setMapping(x, w);
            connect(x, SIGNAL(clicked()), mapper, SLOT(map()));
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(remove_alert(QWidget*)));

            //add alert to list
            _alerts->add(w);

            if(should_alert(_alert_tab_index)) 
                alert_tab(_alert_tab_index);

            ENSURE(_conversations->isVisible());
        }

        void main_window::remove_alert(QWidget* a)
        {
            REQUIRE(a);
            INVARIANT(_alerts);
            _alerts->remove(a);
        }

        void main_window::focus_changed(QWidget* old, QWidget* now)
        {
            INVARIANT(_conversations);
            if(now && old == nullptr && isAncestorOf(now)) 
            {
                _focus = true;
                tab_changed(_conversations->currentIndex());
            }
            else if(old && isAncestorOf(old) && now == nullptr) 
                _focus = false;
        }

        void main_window::new_conversation_event(const std::string& id)
        {
            INVARIANT(_app_service);
            INVARIANT(_conversation_service);
            INVARIANT(_conversations);

            attach_start_screen();

            auto s = _conversation_service->conversation_by_id(id);
            if(!s) return;

            auto sw = new conversation_widget{_conversation_service, s, _app_service};

            std::string name = convert(NEW_CONVERSATION_NAME);

            //make default name to be first person in contact list
            if(!s->contacts().empty()) 
                name = s->contacts().list()[0]->name();

            //create the conversations widget
            sw->name(name.c_str());
            auto tab_index = _conversations->addTab(sw, name.c_str());


            //switch to new tab if initiated by user
            if(s->initiated_by_user()) 
                _conversations->setCurrentIndex(_conversations->count()-1);
            else
            {
                _conversations->setTabTextColor(tab_index, QColor{"red"});
                QApplication::alert(this);
            }

            ENSURE(_conversations->isVisible());
        }

        int find_conversation(QTabWidget* conversations, const std::string& id)
        {
            REQUIRE(conversations);

            //find correct tab
            int ri = -1;
            for(int i = 0; i < conversations->count(); i++)
            {
                auto sw = dynamic_cast<conversation_widget*>(conversations->widget(i));
                if(!sw) continue;

                auto s = sw->conversation();
                CHECK(s);
                if(s->id() == id)
                {
                    ri = i;
                    break;
                }
            }
            return ri;
        }

        void main_window::quit_conversation_event(const std::string& id)
        {
            INVARIANT(_app_service);
            INVARIANT(_conversation_service);
            INVARIANT(_conversations);

            //find correct tab
            int ri = find_conversation(_conversations, id);
            if(ri == -1) return;

            QWidget* w = _conversations->widget(ri);
            _conversations->removeTab(ri);
            if(w) delete w;
        }

        void main_window::conversation_alert_event(const s::event::conversation_alert& e)
        {
            INVARIANT(_conversations);

            auto t = find_conversation(_conversations, e.conversation_id);
            if(!_focus || !e.visible) alert_tab(t);
        }

        void main_window::not_part_of_clique_event(const s::event::not_part_of_clique& e)
        {
            REQUIRE_FALSE(e.conversation_id.empty());
            REQUIRE_FALSE(e.contact_id.empty());
            REQUIRE_FALSE(e.dont_know.empty());
            
            INVARIANT(_conversations);
            INVARIANT(_conversation_service);
            INVARIANT(_user_service);

            auto ci = find_conversation(_conversations, e.conversation_id);
            if(ci == -1) return;

            auto sw = _conversations->widget(ci);
            if(!sw) return;

            auto s = dynamic_cast<conversation_widget*>(sw);
            if(!s) return;

            auto c = _user_service->by_id(e.contact_id);
            if(!c) return;

            std::set<std::string> names;
            for(const auto& id : e.dont_know)
            {
                CHECK_FALSE(id.empty());

                auto dc = _user_service->by_id(id);
                if(!dc) return;

                names.insert(names.end(), dc->name());
            }

            std::stringstream ss;
            ss << " Could not add <b>" << c->name() << "</b> to the conversation because <br/>";

            if(names.size() == 1)
            {
                ss << "<b>" << (*names.begin()) << "</b>" <<  " does not know them";
            }
            else
            {
                size_t i = 0;
                for(const auto& name : names)
                {
                    if(i < names.size() - 1) ss << "<b>" << name << "</b>, ";
                    else ss << " and <b>" << name << "</b>";
                    i++;
                }
                ss << " do not know them";
            }

            QMessageBox::warning(this, tr("Could Not Add To Conversation"), ss.str().c_str());

        }

        void main_window::contact_removed_or_added_from_conversation_event(const m::message& e)
        {
            _conversation_service->broadcast_message(e);
        }

        void main_window::contact_connected_event(const us::event::contact_connected& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_conversation_service);
            INVARIANT(_start_contacts);

            //update start contact list
            _start_contacts->update(_user_service->user().contacts());
            _start_contacts->update_status(true);

            //get user
            auto c = _user_service->user().contacts().by_id(r.id);
            if(!c) return;

            //setup alert widget
            std::stringstream s;
            s << "<b>" << c->name() << "</b> is <font color='green'>online</font>";

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            auto t = new QLabel{tr(s.str().c_str())};
            l->addWidget(t);

            //display alert
            show_alert(w);
            _conversation_service->broadcast_message(r.to_message());
        }

        void main_window::contact_disconnected_event(const us::event::contact_disconnected& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_start_contacts);

            //update start contact list
            _start_contacts->update(_user_service->user().contacts());
            _start_contacts->update_status(true);

            //setup alert widget
            std::stringstream s;
            s << "<b>" << r.name << "</b> has <font color='red'>disconnected</font>";

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);
            auto t = new QLabel{tr(s.str().c_str())};
            l->addWidget(t);

            //display alert
            show_alert(w);
            _conversation_service->broadcast_message(r.to_message());
        }

        void main_window::new_intro_event(const user::event::new_introduction& i)
        {
            INVARIANT(_user_service);
            auto is = _user_service->introductions();
            if(i.index < 0 || i.index >= is.size()) return;

            auto in = is[i.index];
            auto from = _user_service->by_id(in.from_id);
            if(!from) return;

            //setup alert widget
            std::stringstream s;
            s << "<b>" << from->name() << "</b> wants to introduce you to <b>" << in.contact.name() << "</b>" << std::endl;

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);
            auto t = new QLabel{tr(s.str().c_str())};
            l->addWidget(t);

            //display alert
            show_alert(w);
        }

        void main_window::apps_updated_event(const app::event::apps_updated&)
        {
            update_app_menu();
        }
    }
}
