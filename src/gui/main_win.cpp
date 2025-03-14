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
#include "gui/main_win.hpp"

#include "gui/app/chat.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"


#include "gui/contact_list.hpp"
#include "gui/conversation.hpp"
#include "gui/debug_win.hpp"
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
#include "util/version.hpp"

#include <sstream>
#include <stdexcept>

#include <QtWidgets>
#include <QSignalMapper>
#include <QDesktopServices>
#include <QDir>

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
            const std::string WEB_URL = "http://firestr.com";
            const std::string DOC_URL = "http://fire.readthedocs.org/en/latest/";
            const std::string DOC_API_URL = "http://fire.readthedocs.org/en/latest/api/reference/";
            const std::string EXAMPLE_APP_DIR = ":examples/";

            const QString NEW_CONVERSATION_NAME = "new"; 
            const int MAIN_X = 80;
            const int MAIN_Y = 80;
            const int MAIN_W = 480;
            const int MAIN_H = 640;
            const int ALERT_DURATION = 3000; //3 sec
        }
        
        main_tabs::main_tabs(QWidget* parent): QTabWidget{parent}
        {
            setDocumentMode(true);
            setTabPosition(QTabWidget::West);
        }

        void main_tabs::alert_tab(int i)
        {
            REQUIRE_GREATER_EQUAL(i, 0);

            if(i >= static_cast<int>(_as.size()))
                _as.resize(i+1, nullptr);

            if(!_as[i])
            {
                _as.resize(i+1, nullptr);

                auto a = new QPropertyAnimation{this, "tab_color"};
                a->setDuration(600);
                a->setKeyValueAt(0, QColor{255, 0, 0, i});
                a->setKeyValueAt(0.50, QColor{255, 255, 255, i});
                a->setKeyValueAt(1.0, QColor{254, 0, 0, i});
                a->start();
                
                _as[i] = a;
            }

            CHECK(_as[i]);
            _as[i]->start();
        }

        void main_tabs::clear_alert(int i)
        {
            INVARIANT(tabBar());

            if(i < static_cast<int>(_as.size()) && _as[i])
                _as[i]->stop();

            tabBar()->setTabTextColor(i, QColor{"black"});
        }

        QColor main_tabs::tab_color() 
        {
            return QColor{"black"};
        }

        void main_tabs::set_tab_color(const QColor& c) 
        { 
            INVARIANT(tabBar());

            int i = c.alpha();
            QColor act{c.red(), c.green(), c.blue()};
            tabBar()->setTabTextColor(i, act);
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
            create_menus();
            create_main();
            restore_state();
            setup_timers();
            setup_defaults();

            INVARIANT(_master);
            INVARIANT(_main_menu);
        }

        main_window::~main_window()
        {
            INVARIANT(_mail_service);
            INVARIANT(_app_reaper);
            _app_reaper->stop();
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

        bool has_app(const QList<QUrl>& urls)
        {
            for(const auto& url : urls)
                if(url.toLocalFile().endsWith(".fab"))
                    return true;
            return false;
        }

        void main_window::dragEnterEvent(QDragEnterEvent* e)
        {
            REQUIRE(e);
            auto md = e->mimeData();
            if(!md) return;
            if(has_app(md->urls())) e->acceptProposedAction();
        }

        void main_window::dropEvent(QDropEvent* e)
        {
            REQUIRE(e);
            INVARIANT(_start_contacts);
            const auto* md = e->mimeData();
            if(!md) return;
            if(!md->hasUrls()) return;

            auto urls = md->urls();
            for(const auto& url : urls)
            {
                auto local = url.toLocalFile();
                if(local.endsWith(".fab"))
                    install_app(convert(local));
            }
        }

        void main_window::setup_post()
        {
            REQUIRE(!_master);

            //setup the encrypted channels which will handle security conversations
            //with contacts
            _encrypted_channels = std::make_shared<sc::encrypted_channels>(_context.user->private_key());

            //create post office to handle incoming and outgoing messages
            _master = std::make_shared<m::master_post_office>(
                    n::get_lan_ip(_context.host), 
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
            _conversations = new main_tabs;
            _layout->addWidget(_conversations);
            connect(_conversations, SIGNAL(currentChanged(int)), this, SLOT(tab_changed(int)));
            connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(focus_changed(QWidget*,QWidget*)));

            create_alert_screen();
            create_welcome_screen(false); //don't force
            create_contacts_screen();

            std::string title = "Fire★ - " + _user_service->user().info().name();
            //setup base
            setWindowTitle(tr(title.c_str()));
            setCentralWidget(_root);

            tab_changed(-1);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_conversations);
            ENSURE(_contacts_screen);
        }

        void main_window::create_welcome_screen(bool force)
        {
            // don't create welcome screen if there is a contact
            if(!force && !_user_service->user().contacts().empty())
                return;

            _welcome_screen = new QWidget;
            auto l = new QGridLayout;
            _welcome_screen->setLayout(l);

            auto step1 = new QLabel(
                    tr(
                        "<h2>1. <font color='green'>Share Your Identity</font></h2>"
                        "Give your identity to others any way you want.<br>"
                      ));
            auto step1b = new QPushButton(tr("Show Identity"));
            make_big_identity(*step1b);
            step1b->setToolTip(tr("Show Identity"));

            auto step2 = new QLabel(
                    tr(
                        "<h2>2. <font color='green'>Add People</font></h2>"
                        "Copy their identity and paste it.<br>"
                      ));

            auto step2b = new QPushButton(tr("Add People"));
            make_big_add_contact(*step2b);
            step2b->setToolTip(tr("Add People"));

            auto step3 = new QLabel(
                    tr(
                        "<h2>3. <font color='green'>Start a Conversation</font></h2>"
                        "Start a conversation when you are connected.<br>"
                      ));

            auto step3b = new QPushButton(tr("Start Conversation"));
            make_big_new_conversation(*step3b);
            step3b->setToolTip(tr("Start Conversation"));

            //step 1
            l->addWidget(step1, 0, 0);
            l->addWidget(step1b, 0, 1);

            //step3
            l->addWidget(step2, 1, 0);
            l->addWidget(step2b, 1, 1);

            //step 4
            l->addWidget(step3, 2, 0);
            l->addWidget(step3b, 2, 1);

            connect(step1b, SIGNAL(clicked()), this, SLOT(show_identity()));
            connect(step2b, SIGNAL(clicked()), this, SLOT(add_contact()));
            connect(step3b, SIGNAL(clicked()), this, SLOT(create_conversation()));

            _conversations->addTab(_welcome_screen, "Getting Started");

            ENSURE(_welcome_screen);
        }

        void main_window::create_contacts_screen()
        {
            REQUIRE(_layout);
            REQUIRE(_user_service);
            REQUIRE_FALSE(_contacts_screen);

            _contacts_screen = new QWidget;

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
                    connect(mapper, SIGNAL(mappedString(QString)), this, SLOT(create_conversation(QString)));

                    return new user_info{u, _user_service, con, true};
                }
            };

            _start_contacts->update_status(true);


            auto right = new QWidget;
            auto rl = new QVBoxLayout;
            right->setLayout(rl);

            auto iden = new QPushButton(tr("Show Identity"));
            make_big_identity(*iden);
            iden->setToolTip(tr("Show Identity"));

            auto add = new QPushButton(tr("Add People"));
            make_big_add_contact(*add);
            add->setToolTip(tr("Add People"));

            auto convo = new QPushButton(tr("Start Conversation"));
            make_big_new_conversation(*convo);
            convo->setToolTip(tr("Start Conversation"));

            rl->addWidget(iden);
            rl->addWidget(add);
            rl->addWidget(convo);

            connect(iden, SIGNAL(clicked()), this, SLOT(show_identity()));
            connect(add, SIGNAL(clicked()), this, SLOT(add_contact()));
            connect(convo, SIGNAL(clicked()), this, SLOT(create_conversation()));

            auto l = new QHBoxLayout;
            l->addWidget(_start_contacts);
            l->addWidget(right);

            _contacts_screen->setLayout(l);

            _contacts_tab_index = _conversations->addTab(_contacts_screen, "People");
            ENSURE(_contacts_screen);
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
            REQUIRE_FALSE(_help_menu);
            REQUIRE_FALSE(_debug_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);
            REQUIRE(_show_identity_action);
            REQUIRE(_email_invite_action);
            REQUIRE(_add_contact_action);
            REQUIRE(_contact_list_action);
            REQUIRE(_chat_app_action);
            REQUIRE(_app_editor_action);
            REQUIRE(_create_conversation_action);
            REQUIRE(_install_app_action);
            REQUIRE(_remove_app_action);
            REQUIRE(_show_getting_started_action);
            REQUIRE(_open_docs_action);
            REQUIRE(_open_api_action);
            REQUIRE(_open_website_action);

            _main_menu = new QMenu{tr("&Main"), this};
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_install_app_action);
            _main_menu->addAction(_remove_app_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            _contact_menu = new QMenu{tr("&People"), this};
            _contact_menu->addAction(_add_contact_action);
            _contact_menu->addSeparator();
            _contact_menu->addAction(_show_identity_action);
            _contact_menu->addAction(_email_invite_action);
            _contact_menu->addSeparator();
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

            _help_menu = new QMenu{tr("&Help"), this};
            _help_menu->addAction(_open_website_action);
            _help_menu->addAction(_open_docs_action);
            _help_menu->addAction(_open_api_action);
            _help_menu->addAction(_show_getting_started_action);

            menuBar()->addMenu(_main_menu);
            menuBar()->addMenu(_contact_menu);
            menuBar()->addMenu(_conversation_menu);
            menuBar()->addMenu(_app_menu);
            if(_debug_menu) menuBar()->addMenu(_debug_menu);
            menuBar()->addMenu(_help_menu);
            menuBar()->setHidden(false);

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
            update_remove_app_menu();
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
                connect(mapper, SIGNAL(mappedString(QString)), this, SLOT(load_app_into_conversation(QString)));

                _app_menu->addAction(action);
            }
        }

        void main_window::update_remove_app_menu()
        {
            INVARIANT(_remove_app_action);
            INVARIANT(_app_service);
            _remove_app_action->setEnabled(!_app_service->available_apps().empty());
        }

        void main_window::setup_timers()
        {
            INVARIANT(_mail);
            _mail_service = new mail_service{_mail, this};
            _mail_service->start();
            ENSURE(_mail_service);
        }

        void main_window::setup_defaults()
        try
        {
            INVARIANT(_user_service);
            INVARIANT(_app_service);

            //if the user has been already created, skip defaults
            //as they should have been created the first time.
            if(!_context.user_just_created) return;

            add_default_greeter(*_user_service);
            install_example_apps();
        }
        catch(std::exception& e)
        {
            LOG << "Error configuring defaults: " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "Unexpected error configuring defaults." << std::endl;
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

            _email_invite_action = new QAction{tr("&Email Identity"), this};
            connect(_email_invite_action, SIGNAL(triggered()), this, SLOT(email_invite()));

            _show_identity_action = new QAction{tr("&Show Identity"), this};
            connect(_show_identity_action, SIGNAL(triggered()), this, SLOT(show_identity()));

            _add_contact_action = new QAction{tr("&Add People"), this};
            connect(_add_contact_action, SIGNAL(triggered()), this, SLOT(add_contact()));

            _contact_list_action = new QAction{tr("&Manage..."), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _chat_app_action = new QAction{tr("&Chat"), this};
            connect(_chat_app_action, SIGNAL(triggered()), this, SLOT(make_chat_app()));

            _app_editor_action = new QAction{tr("&App Editor"), this};
            connect(_app_editor_action, SIGNAL(triggered()), this, SLOT(make_app_editor()));

            _install_app_action = new QAction{tr("&Install App"), this};
            connect(_install_app_action, SIGNAL(triggered()), this, SLOT(install_app()));

            _remove_app_action = new QAction{tr("&Remove App"), this};
            connect(_remove_app_action, SIGNAL(triggered()), this, SLOT(remove_app()));

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

            _open_website_action = new QAction{tr("&Website"), this};
            connect(_open_website_action, SIGNAL(triggered()), this, SLOT(open_website()));

            _open_docs_action = new QAction{tr("&Documentation"), this};
            connect(_open_docs_action, SIGNAL(triggered()), this, SLOT(open_docs()));

            _open_api_action = new QAction{tr("&API Reference"), this};
            connect(_open_api_action, SIGNAL(triggered()), this, SLOT(open_api()));

            _show_getting_started_action = new QAction{tr("&Show Getting Started Tab"), this};
            connect(_show_getting_started_action, SIGNAL(triggered()), this, SLOT(show_welcome_screen()));

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_show_identity_action);
            ENSURE(_email_invite_action);
            ENSURE(_contact_list_action);
            ENSURE(_create_conversation_action);
            ENSURE(_rename_conversation_action);
            ENSURE(_quit_conversation_action);
            ENSURE(_chat_app_action);
            ENSURE(_app_editor_action);
            ENSURE(_install_app_action);
            ENSURE(_remove_app_action);
            ENSURE(_open_docs_action);
            ENSURE(_open_api_action);
            ENSURE(_open_website_action);
            ENSURE(_show_getting_started_action);
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
            _app_reaper = std::make_shared<a::app_reaper>(this);

            ENSURE(_user_service);
            ENSURE(_conversation_service);
            ENSURE(_encrypted_channels);
            ENSURE(_app_service);
        }

        void main_window::email_invite()
        {
            INVARIANT(_user_service);
            email_identity(_user_service, this);
        }

        void main_window::show_identity()
        {
            INVARIANT(_user_service);
            gui::show_identity_gui(_user_service, this);
        }

        void main_window::add_contact()
        {
            INVARIANT(_user_service);
            INVARIANT(_start_contacts);

            if(gui::add_contact_gui(_user_service, this))
            {
                _start_contacts->update(_user_service->user().contacts());
                _start_contacts->update_status(true);
            }
        }

        void main_window::show_contact_list()
        {
            INVARIANT(_user_service);

            contact_list_dialog cl{"people", _user_service, this};
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

        bool ask_user_to_select_app(QWidget* w, const a::app_service& apps, const std::string& no_select, std::string& id, std::string& name)
        {
            QStringList as;
            if(!no_select.empty())
                as << no_select.c_str();
            for(const auto& a : apps.available_apps())
                as << a.second.name.c_str();

            bool ok;
            auto g = QInputDialog::getItem(w, w->tr("Select App"),
                    w->tr("Choose an app:"), as, 0, false, &ok);

            if (!ok || g.isEmpty()) return false;

            name = convert(g);
            if(name == no_select) id = "";
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
            std::string name; 
            if(!ask_user_to_select_app(this, *_app_service, NEW_APP_S, id, name)) return;

            s->add_app_editor(id);
        }

        void main_window::install_example_app(const std::string& resource_path)
        {
            INVARIANT(_app_service);

            auto bytes = gui::get_resource_as_bytes(resource_path);
            auto decoded_bytes = u::decode<u::bytes>(bytes); //file is stored in mencoded bytes form
            auto msg = a::import_app_as_message(decoded_bytes);
            auto app = _app_service->create_app(msg);

            if(!app) 
            {
                LOG << "error installing " << resource_path << std::endl;
                return;
            }

            _app_service->save_app(*app);

            LOG << "installed " << resource_path << std::endl;
        }

        void main_window::install_example_apps()
        {
            INVARIANT(_app_service);
            auto example_dir = QDir{EXAMPLE_APP_DIR.c_str()}; 
            auto examples = example_dir.entryList(); 
            for(auto e = examples.constBegin(); e != examples.constEnd(); e++) 
            { 
                std::stringstream full_path;
                full_path << EXAMPLE_APP_DIR << gui::convert(*e);

                install_example_app(full_path.str());
            }
        }

        void main_window::install_app(const std::string& file)
        {
            INVARIANT(_app_service);
            REQUIRE_FALSE(file.empty());

            auto app = _app_service->import_app(file);
            CHECK(app);

            bool installed = install_app_gui(*app, *_app_service, this);

            if(installed)
            {
                std::stringstream ss;
                ss << "`" << app->name() << "' has been installed";
                QMessageBox::information(this, tr("App Installed"), ss.str().c_str());
            }
        }

        void main_window::remove_app()
        {
            INVARIANT(_app_service);

            std::string id; 
            std::string name; 
            if(!ask_user_to_select_app(this, *_app_service, "", id, name)) return;

            CHECK_FALSE(id.empty());

            bool removed = _app_service->remove_app(id);
            std::stringstream ss;
            if(removed)
            {
                ss << "`" << name << "' has been removed";
                QMessageBox::information(this, tr("App Removed"), ss.str().c_str());
            }
            else
            {
                ss << "Error removing `" << name << "'";
                QMessageBox::critical(this, tr("Error Removing App"), ss.str().c_str());
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
            auto version = u::version_string();

            std::stringstream ls;
            ls << "Fire★ " << version;

            std::stringstream ss;
            ss << "<p><b>Fire★</b> is a simple distributed communication and computation "
                        "platform. Write, clone, modify, and send people programs which "
                        "communicate with each other automatically, in a distributed way.</p>"
                        "<p>This is not the web, but it is on the internet.<br> "
                        "This is not a chat program, but a way for programs to chat.<br> "
                        "This is not just a way to share code, but a way to share running software.</p> "
                        "<p>This program is created by <b>Maxim Noah Khailo</b> and is licensed as GPLv3</p><br>";
            ss << "<br> <b>Version</b>: " << version << "<br>";
            QMessageBox::about(this, ls.str().c_str(), ss.str().c_str());
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
            _service_map.handle(us::event::CONTACT_ACTIVITY_CHANGED,
                    bind(&main_window::received_contact_activity_changed, this, _1));
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

        void main_window::received_contact_activity_changed(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_ACTIVITY_CHANGED);

            us::event::contact_activity_changed r;
            r.from_message(m);

            contact_activity_changed_event(r);
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
            LOG << "Error receiving message in `" << _mail->address() << "'. " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "Unexpected error receiving message in `" << _mail->address() << "'" << std::endl;
        }

        void main_window::tab_changed(int i)
        {
            INVARIANT(_create_conversation_action);
            INVARIANT(_rename_conversation_action);
            INVARIANT(_quit_conversation_action);
            INVARIANT(_conversations);
            INVARIANT(_app_menu);

            _alert_tab_index = _conversations->indexOf(_alert_screen);

            conversation_widget* s = nullptr;

            auto sw = _conversations->widget(i);
            if(i != -1 && sw != nullptr) s = dynamic_cast<conversation_widget*>(sw);

            bool enabled = s != nullptr;

            _rename_conversation_action->setEnabled(enabled);
            _quit_conversation_action->setEnabled(enabled);
            _app_menu->setEnabled(enabled);

            if(s) s->clear_alerts();

            if(i != -1 && (i == _alert_tab_index || i == _contacts_tab_index || enabled))
                _conversations->clear_alert(i);
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

        void main_window::app_alert()
        {
            QApplication::alert(this, ALERT_DURATION);
        }

        void main_window::alert_tab(int tab_index)
        {
            INVARIANT(_conversations);
            _conversations->alert_tab(tab_index);
            app_alert();
        }

        void main_window::show_alert(QWidget* a)
        {
            REQUIRE(a);
            INVARIANT(_alert_screen);
            INVARIANT(_alerts);

            if(!_alert_screen->isVisible())
            {
                CHECK(_conversations->isVisible());

                _alert_screen->show();
                _alert_tab_index = _conversations->addTab(_alert_screen, tr("Alert"));
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
            connect(mapper, SIGNAL(mappedObject(QWidget*)), this, SLOT(remove_alert(QWidget*)));

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

        void main_window::open_website()
        {
            QDesktopServices::openUrl(QUrl(WEB_URL.c_str()));
        }
        
        void main_window::open_docs()
        {
            QDesktopServices::openUrl(QUrl(DOC_URL.c_str()));
        }

        void main_window::open_api()
        {
            QDesktopServices::openUrl(QUrl(DOC_API_URL.c_str()));
        }

        void main_window::show_welcome_screen()
        {
            if(_welcome_screen) return;
            create_welcome_screen(true);
        }

        void main_window::new_conversation_event(const std::string& id)
        {
            INVARIANT(_app_service);
            INVARIANT(_conversation_service);
            INVARIANT(_conversations);

            auto s = _conversation_service->conversation_by_id(id);
            if(!s) return;

            auto sw = new conversation_widget{_conversation_service, s, _app_service, _app_reaper};

            //if the conversation was initiated by the user
            //Then add a chat app and sync the conversation
            if(s->initiated_by_user()) 
            {
                sw->add_chat_app();
                _conversation_service->sync_existing_conversation(s);
            }

            std::string name = convert(NEW_CONVERSATION_NAME);

            //make default name to be first person in contact list
            if(!s->contacts().empty()) 
                name = s->contacts().list()[0]->name();

            //create the conversations widget
            sw->name(name.c_str());
            auto tab_index = _conversations->addTab(sw, name.c_str());
            //switch to new tab if initiated by user
            if(s->initiated_by_user()) 
                _conversations->setCurrentIndex(tab_index);
            else
                alert_tab(tab_index);

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
            if(should_alert(t)) alert_tab(t);
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

            //broadcast to conversations
            _conversation_service->broadcast_message(r.to_message());

            if(should_alert(_contacts_tab_index)) 
                alert_tab(_contacts_tab_index);
        }

        void main_window::contact_disconnected_event(const us::event::contact_disconnected& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_start_contacts);

            //update start contact list
            _start_contacts->update(_user_service->user().contacts());
            _start_contacts->update_status(true);

            //broadcast to conversations
            _conversation_service->broadcast_message(r.to_message());

            if(should_alert(_contacts_tab_index)) 
                alert_tab(_contacts_tab_index);
        }

        void main_window::contact_activity_changed_event(const us::event::contact_activity_changed& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_start_contacts);

            //update status of exising contacts
            _start_contacts->update_status(true);

            //broadcast to conversations
            _conversation_service->broadcast_message(r.to_message());
        }

        void main_window::new_intro_event(const user::event::new_introduction& i)
        {
            INVARIANT(_user_service);
            auto is = _user_service->introductions();
            if(i.index >= is.size()) return;

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
            update_remove_app_menu();
        }
    }
}
