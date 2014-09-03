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

#include "util/mencode.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"
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
            _create_conversation_action{0},
            _rename_conversation_action{0},
            _quit_conversation_action{0},
            _debug_window_action{0},
            _main_menu{0},
            _contact_menu{0},
            _app_menu{0},
            _debug_menu{0},
            _root{0},
            _layout{0},
            _conversations{0},
            _focus{true},
            _context(c)
        {
            setWindowIcon(logo());
            setAcceptDrops(true);
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

        bool has_finvite(const QList<QUrl>& urls)
        {
            for(const auto& url : urls)
                if(url.toLocalFile().endsWith(".finvite"))
                    return true;
            return false;
        }

        void main_window::dragEnterEvent(QDragEnterEvent* e)
        {
            REQUIRE(e);
            auto md = e->mimeData();
            if(!md) return;
            if(has_finvite(md->urls())) e->acceptProposedAction();
        }

        void main_window::dropEvent(QDropEvent* e)
        {
            REQUIRE(e);
            const auto* md = e->mimeData();
            if(!md) return;
            if(!md->hasUrls()) return;

            auto urls = md->urls();
            contact_list_dialog cl{"contacts", _user_service, this};
            for(const auto& url : urls)
            {
                auto local = url.toLocalFile();
                if(!local.endsWith(".finvite")) continue;
#ifdef _WIN64
                auto cf = convert16(local);
#else
                auto cf = convert(local);
#endif
                cl.new_contact(cf, true);

            }

            cl.exec();
            cl.save_state();
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
                auto add_contact = new QPushButton(tr("connect with someone"));

                auto intro2 = new QLabel(tr("Once connected, start a conversation"));
                auto add_conversation = new QPushButton(tr("start conversation"));
                l->addWidget(intro);
                l->addWidget(add_contact);
                l->addWidget(intro2);
                l->addWidget(add_conversation);

                connect(add_contact, SIGNAL(clicked()), this, SLOT(show_contact_list()));
                connect(add_conversation, SIGNAL(clicked()), this, SLOT(create_conversation()));
            }
            else
            {
                auto intro = new QLabel(
                        tr(
                        "<b>Welcome!</b><br><br>"
                        "Start by creating a conversation"
                        ));
                auto add_conversation = new QPushButton(tr("create conversation"));
                l->addWidget(intro);
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
            REQUIRE(_contact_list_action);
            REQUIRE(_chat_app_action);
            REQUIRE(_app_editor_action);
            REQUIRE(_create_conversation_action);

            _main_menu = new QMenu{tr("&Main"), this};
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            _contact_menu = new QMenu{tr("&Contacts"), this};
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

            _contact_list_action = new QAction{tr("&Contacts"), this};
            connect(_contact_list_action, SIGNAL(triggered()), this, SLOT(show_contact_list()));

            _chat_app_action = new QAction{tr("&Chat"), this};
            connect(_chat_app_action, SIGNAL(triggered()), this, SLOT(make_chat_app()));

            _app_editor_action = new QAction{tr("&App Editor"), this};
            connect(_app_editor_action, SIGNAL(triggered()), this, SLOT(make_app_editor()));

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
            ENSURE(_contact_list_action);
            ENSURE(_create_conversation_action);
            ENSURE(_rename_conversation_action);
            ENSURE(_quit_conversation_action);
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

        void main_window::show_contact_list()
        {
            ENSURE(_user_service);

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

        void main_window::load_app_into_conversation(QString qid)
        {
            INVARIANT(_app_service);
            INVARIANT(_conversations);
            INVARIANT(_conversation_service);

            //get current conversation
            auto s = dynamic_cast<conversation_widget*>(_conversations->currentWidget());
            if(!s) return;

            //load app
            auto id = convert(qid);
            s->add_script_app(id);
        }

        void main_window::about()
        {
            QMessageBox::about(this, tr("Firestr 0.3"),
                    tr("<p><b>Fire★</b> is a simple distributed communication and computation "
                        "platform. Write, clone, modify, and send people programs which "
                        "communicate with each other automatically, in a distributed way.</p>"
                        "<p>This is not the web, but it is on the internet.<br> "
                        "This is not a chat program, but a way for programs to chat.<br> "
                        "This is not just a way to share code, but a way to share running software.</p> "
                        "<p>This program is created by <b>Maxim Noah Khailo</b> and is licensed as GPLv3</p>"));
        }

        void main_window::check_mail(m::message m)
        try
        {
            INVARIANT(_mail);

            if(m.meta.type == s::event::NEW_CONVERSATION)
            {
                s::event::new_conversation r;
                s::event::convert(m, r);

                new_conversation_event(r.conversation_id);
            }
            else if(m.meta.type == s::event::QUIT_CONVERSATION)
            {
                s::event::quit_conversation r;
                s::event::convert(m, r);

                quit_conversation_event(r.conversation_id);
            }
            else if(m.meta.type == s::event::CONVERSATION_SYNCED)
            {
                conversation_synced_event(m);
            }
            else if(m.meta.type == s::event::CONTACT_REMOVED || m.meta.type == s::event::CONTACT_ADDED)
            {
                contact_removed_or_added_from_conversation_event(m);
            }
            else if(m.meta.type == s::event::CONVERSATION_ALERT)
            {
                s::event::conversation_alert e;
                s::event::convert(m, e);
                conversation_alert_event(e);
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
            else if(m.meta.type == us::event::NEW_INTRODUCTION)
            {
                us::event::new_introduction n;
                us::event::convert(m, n);
                new_intro_event(n);
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

        void main_window::conversation_synced_event(const m::message& m)
        {
            INVARIANT(_conversation_service);

            s::event::conversation_synced e;
            s::event::convert(m, e);

            auto s = _conversation_service->conversation_by_id(e.conversation_id);
            if(!s) return;

            CHECK(s->mail());
            s->mail()->push_inbox(m);
        }

        void main_window::conversation_alert_event(const s::event::conversation_alert& e)
        {
            INVARIANT(_conversations);

            auto t = find_conversation(_conversations, e.conversation_id);
            if(!_focus || !e.visible) alert_tab(t);
        }

        void main_window::contact_removed_or_added_from_conversation_event(const m::message& e)
        {
            _conversation_service->broadcast_message(e);
        }

        void main_window::contact_connected_event(const us::event::contact_connected& r)
        {
            INVARIANT(_user_service);
            INVARIANT(_conversation_service);

            //get user
            auto c = _user_service->user().contacts().by_id(r.id);
            if(!c) return;

            //setup alert widget
            std::stringstream s;
            s << "<b>" << c->name() << "</b> is online" << std::endl;

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            auto t = new QLabel{tr(s.str().c_str())};
            l->addWidget(t);

            auto b = new QPushButton{tr("new conversation")};
            l->addWidget(b);
            auto m = new QSignalMapper{w};

            m->setMapping(b, QString{r.id.c_str()});
            connect(b, SIGNAL(clicked()), m, SLOT(map()));
            connect(m, SIGNAL(mapped(QString)), this, SLOT(create_conversation(QString)));

            //display alert
            show_alert(w);
            _conversation_service->broadcast_message(us::event::convert(r));
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
            auto t = new QLabel{tr(s.str().c_str())};
            l->addWidget(t);

            //display alert
            show_alert(w);
            _conversation_service->broadcast_message(us::event::convert(r));
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
