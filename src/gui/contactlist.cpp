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

#include "gui/contactlist.hpp"
#include "gui/util.hpp"
#include "network/message_queue.hpp"
#include "util/dbc.hpp"
#include "util/env.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <QtWidgets>
#include <QGridLayout>
#include <QLabel>
#include <QDesktopServices>
#include <QByteArray>
#include <QDir>
#include <QClipboard>

#include <cstdlib>
#include <sstream>

namespace u = fire::util;
namespace n = fire::network;
namespace us = fire::user;
                
namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 5000;//in milliseconds
            const char* GREETER_TIP = "A locator helps connect you with your contacts.\nTry adding 'mempko.com:8080' and ask them to do the same.";
            const std::string DEFAULT_GREETER = "mempko.com:8080";
        }

        bool is_online(
                us::user_info_ptr c, 
                us::user_service_ptr s, 
                bool force_offline = false)
        {
            REQUIRE(c);
            REQUIRE(s);
            return !force_offline && s->contact_available(c->id());
        }

        std::string user_text(
                us::user_info_ptr c, 
                bool online,
                const us::contact_version& version)
        {
            REQUIRE(c);
            std::stringstream ss;
            std::string color = "red";
            if(online)
            {
                color = "green";
                if(version.client != u::CLIENT_VERSION)
                    color = "blue";

                if(version.protocol != u::PROTOCOL_VERSION)
                    color = "purple";
            }
            ss << "<font color='" << color << "'>" << c->name() << "</font>";
            return ss.str();
        }

        std::string user_tooltip(
                us::user_info_ptr c, 
                bool online,
                const us::contact_version& version)
        {
            REQUIRE(c);
            std::string text = "offline";
            if(online)
            {
                text = "online ";

                if(version.client < u::CLIENT_VERSION)
                    text = "older client ";
                else if(version.client > u::CLIENT_VERSION)
                    text = "newer client ";

                if(version.protocol < u::PROTOCOL_VERSION)
                    text = "older protocol ";
                if(version.protocol > u::PROTOCOL_VERSION)
                    text = "newer protocol ";

                std::stringstream ss;
                ss << "(" << version.protocol << "." << version.client << ")";
                text += ss.str();
            }

            return text;
        }

        user_info::user_info(
                us::user_info_ptr p, 
                us::user_service_ptr s,
                QPushButton* action) :
            _contact{p},
            _service{s},
            _action{action}
        {
            REQUIRE(p)
            REQUIRE(s)

            auto* layout = new QGridLayout{this};
            setLayout(layout);

            bool online = is_online(p, _service);
            auto version = s->check_contact_version(p->id());
            
            _user_text = new QLabel{user_text(p, online, version).c_str()};
            _user_text->setToolTip(user_tooltip(p, online, version).c_str());

            layout->addWidget(_user_text, 0,0);
            if(_action) layout->addWidget(_action, 0,1);

            layout->setContentsMargins(2,2,2,2);
            ENSURE(_contact);
            ENSURE(_user_text);
        }

        void user_info::update(bool update_action)
        {
            INVARIANT(_service);
            INVARIANT(_contact);
            INVARIANT(_user_text);

            bool online = is_online(_contact, _service);
            auto version = _service->check_contact_version(_contact->id());
            _user_text->setText(user_text(_contact, online, version).c_str());
            _user_text->setToolTip(user_tooltip(_contact, online, version).c_str());

            if(_action && update_action) _action->setVisible(online);
        }

        void user_info::update(std::function<bool(us::user_info&)> f, bool update_action)
        {
            INVARIANT(_service);
            INVARIANT(_contact);
            INVARIANT(_user_text);

            bool online = is_online(_contact, _service , !f(*_contact));
            auto version = _service->check_contact_version(_contact->id());
            _user_text->setText(user_text(_contact, online, version).c_str());
            _user_text->setToolTip(user_tooltip(_contact, online, version).c_str());

            if(_action && update_action) _action->setVisible(online);
        }

        contact_list_dialog::contact_list_dialog(
                const std::string& title, 
                us::user_service_ptr service,
                QWidget* parent) :
            QDialog{parent},
            _service{service}
        {
            REQUIRE(service);

            //create tabs
            auto* layout = new QVBoxLayout{this};
            setLayout(layout);
            auto* tabs = new QTabWidget{this};
            layout->addWidget(tabs);
            
            auto* contacts_tab = new QWidget;
            auto* contacts_layout = new QGridLayout{contacts_tab};

            auto* intro_tab = new QWidget;
            auto* intro_layout = new QGridLayout{intro_tab};

            auto* greeters_tab = new QWidget;
            auto* greeters_layout = new QGridLayout{greeters_tab};
            if(_service->user().greeters().empty())
            {
                tabs->addTab(greeters_tab, tr("locators"));
                tabs->addTab(contacts_tab, tr("contacts"));
            }
            else
            {
                tabs->addTab(contacts_tab, tr("contacts"));
                tabs->addTab(greeters_tab, tr("locators"));
            }

            tabs->addTab(intro_tab, tr("introductions"));
            if(!_service->user().introductions().empty())
                    tabs->tabBar()->setTabTextColor(2, QColor{"red"});

            init_greeters_tab(greeters_tab, greeters_layout);
            init_contacts_tab(contacts_tab, contacts_layout);
            init_intro_tab(intro_tab, intro_layout);

            setWindowTitle(tr(title.c_str()));
            restore_state();

            INVARIANT(_list);
        }

        void contact_list_dialog::init_contacts_tab(QWidget* tab, QGridLayout* layout)
        {
            REQUIRE(tab);
            REQUIRE(layout);
            INVARIANT(_service);

            //create contact list
            _list = new list;
            layout->addWidget(_list, 0, 0, 2, 3);

            //create add button
            auto add_new = new QPushButton;
            make_add_contact(*add_new);
            add_new->setToolTip(tr("add someone using their invite file"));
            layout->addWidget(add_new, 2, 0, 1, 3); 
            connect(add_new, SIGNAL(clicked()), this, SLOT(new_contact()));

            update_contacts();

            //setup updated timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update()));
            t->start(TIMER_SLEEP);
        }

        void contact_list_dialog::init_intro_tab(QWidget* tab, QGridLayout* layout)
        {
            REQUIRE(tab);
            REQUIRE(layout);
            INVARIANT(_service);
            auto il = new intro_list{_service};

            layout->addWidget(il, 0,0);
            auto introduce = new QPushButton;
            make_introduce(*introduce);
            introduce->setToolTip(tr("Introduce one of your contacts to another.\nThey won't need to exchange invite files."));
            layout->addWidget(introduce, 1,0); 
            connect(introduce, SIGNAL(clicked()), il, SLOT(introduce()));
        }

        void contact_list_dialog::init_greeters_tab(QWidget* tab, QGridLayout* layout)
        {
            REQUIRE(tab);
            REQUIRE(layout);
            INVARIANT(_service);
            auto gl = new greeter_list{_service};
            gl->setToolTip(tr(GREETER_TIP));

            layout->addWidget(gl, 0,0);
            auto add_new = new QPushButton;
            make_add_to_list(*add_new);
            add_new->setToolTip(tr(GREETER_TIP));
            layout->addWidget(add_new, 1,0); 
            connect(add_new, SIGNAL(clicked()), gl, SLOT(add_greeter()));
        }

        void contact_list_dialog::update_contacts()
        {
            INVARIANT(_list);
            INVARIANT(_service);

            _list->clear();

            for(auto u : _service->user().contacts().list())
            {
                auto rm = make_x_button();
                std::stringstream ss;
                ss << "Remove `" << u->name() << "'";
                rm->setToolTip(ss.str().c_str());

                auto mapper = new QSignalMapper{this};
                mapper->setMapping(rm, QString(u->id().c_str()));
                connect(rm, SIGNAL(clicked()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(QString)), this, SLOT(remove(QString)));

                _list->add(new user_info{u, _service, rm});
            }

            _prev_contacts = _service->user().contacts().size();
        }

        std::string greet_address(const us::greet_server& s)
        {
            return s.host() + ":" + n::port_to_string(s.port());
        }


        void contact_list_dialog::new_contact()
        {
            INVARIANT(_service);
            add_contact_gui(_service, this);
        }

        void contact_list_dialog::update()
        {
            size_t contacts = _service->user().contacts().size();
            if(contacts == _prev_contacts) 
                return;

            update_contacts();
            _prev_contacts = contacts;
        }

        void contact_list_dialog::save_state()
        {
            INVARIANT(_service);
            QSettings settings("mempko", app_id(_service->user()).c_str());
            settings.setValue("contact_list/geometry", saveGeometry());
        }

        void contact_list_dialog::restore_state()
        {
            INVARIANT(_service);
            QSettings settings("mempko", app_id(_service->user()).c_str());
            restoreGeometry(settings.value("contact_list/geometry").toByteArray());
        }

        void contact_list_dialog::remove(QString qid)
        {
            INVARIANT(_service);

            auto id = convert(qid);
            auto c = _service->by_id(id);
            if(!c) return;

            std::stringstream msg;
            msg << convert(tr("Are you sure you want to remove `")) << c->name() << "'?";
            auto a = QMessageBox::warning(this, tr("Remove Contact?"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
            if(a != QMessageBox::Yes) return;

            _service->remove_contact(id);
            
            update();
        }

        user_info* simple_info(us::user_info_ptr u, us::user_service_ptr s)
        {
            REQUIRE(s);
            REQUIRE(u);
            return new user_info{u, s};
        }

        contact_list::contact_list(us::user_service_ptr service, const us::contact_list& contacts) :
            contact_list(service, contacts, 
                    [=](us::user_info_ptr u) { return simple_info(u, _service);})
        {
        }

        contact_list::contact_list(us::user_service_ptr service, const us::contact_list& contacts, make_user_info mk) :
            _service{service},
            _mk{mk}
        {
            REQUIRE(service);
            for(auto u : contacts.list())
            {
                CHECK(u);
                add_contact(u);
            }

            INVARIANT(_service);
        }

        void contact_list::add_contact(us::user_info_ptr u)
        {
            REQUIRE(u);
            INVARIANT(_service);
            if(_contacts.by_id(u->id())) return;

            auto ui = _mk(u);
            add(ui);
            _contacts.add(u);
            _contact_widgets.push_back(ui);
        }

        void contact_list::update(const us::contact_list& contacts)
        {
            clear();
            _contact_widgets.clear();
            _contacts.clear();
            for(auto c : contacts.list())
            {
                CHECK(c);
                add_contact(c);
            }

            ENSURE_LESS_EQUAL(_contacts.size(), contacts.size());
        }

        void contact_list::update_status(bool update_action)
        {
            for(auto p : _contact_widgets)
            {
                CHECK(p);
                p->update(update_action);
            }
        }

        void contact_list::update_status(std::function<bool(us::user_info&)> f, bool update_action)
        {
            for(auto p : _contact_widgets)
            {
                CHECK(p);
                p->update(f);
            }
        }

        greeter_list::greeter_list(us::user_service_ptr service) :
            _service{service}

        {
            REQUIRE(service);
            INVARIANT(_service);
            for(const auto& s : _service->user().greeters())
                add_greeter(s);
        }
                
        void greeter_list::add_greeter(const us::greet_server& s)
        {
            add(new greeter_info{_service, s});
        }

       std::string add_new_greeter(us::user_service_ptr service, QWidget* parent)
       {
           REQUIRE(service);
           REQUIRE(parent);

            bool ok = false;
            bool error = false;
            std::string address = DEFAULT_GREETER;

            do
            {
                try
                {
                    QString r = QInputDialog::getText(
                            parent, 
                            (error ? parent->tr("Error, try again") : parent->tr("Add New Locator")),
                            parent->tr("Locator Address"),
                            QLineEdit::Normal, address.c_str(), &ok);

                    if(ok && !r.isEmpty()) address = convert(r);
                    else return "";

                    error = false;
                }
                catch(...)
                {
                    error = true;
                }
            }
            while(error);

            //append port if not specified
            if(address.find(":") == std::string::npos) address.append(":7070");

            service->add_greeter(address);
            return address;
       }

        void greeter_list::add_greeter()
        {
            auto address = add_new_greeter(_service, this);
            if(address.empty()) return;

            auto host_port = n::parse_host_port(address);
            us::greet_server ts{host_port.first, host_port.second, ""};
            add_greeter(ts);
        }

        greeter_info::greeter_info(us::user_service_ptr service, const us::greet_server& s) :
            _server{s}, _service{service}
        {
            INVARIANT(_service);

            auto* layout = new QGridLayout{this};
            setLayout(layout);
            _address = _server.host() + ":" + n::port_to_string(_server.port());
            _label = new QLabel{_address.c_str()};
            layout->addWidget( _label, 0,0);

            _rm = make_x_button();
            std::stringstream ss;
            ss << "Remove `" << _address << "'";
            _rm->setToolTip(ss.str().c_str());

            layout->addWidget(_rm, 0,1);
            connect(_rm, SIGNAL(clicked()), this, SLOT(remove()));

        }

        void greeter_info::remove()
        {
            INVARIANT(_service);

            std::stringstream msg;
            msg << "Are you sure you want to remove `" << _address << "'?";
            auto a = QMessageBox::warning(this, tr("Remove Locator?"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
            if(a != QMessageBox::Yes) return;

            _service->remove_greeter(_address);
            _label->setEnabled(false);
            _rm->setEnabled(false);
        }

        intro_list::intro_list(us::user_service_ptr service) :
            _service{service}
        {
            REQUIRE(service);
            INVARIANT(_service);
            for(const auto& intro : _service->user().introductions())
                add(new intro_info{_service, intro});
        }

        void intro_list::introduce()
        {
            INVARIANT(_service);
            introduce_dialog d{convert(tr("introduce")), _service, this};
            d.exec();
        }

        intro_info::intro_info(us::user_service_ptr service, const us::contact_introduction& intro) :
            _intro(intro), _service{service}
        {
            INVARIANT(_service);

            auto from = _service->by_id(_intro.from_id);
            if(!from) return;

            auto* layout = new QGridLayout{this};
            setLayout(layout);

            std::stringstream l;
            l << "<b>" << from->name() << convert(tr("</b> introduces <b>")) << _intro.contact.name() << "</b>";
            _label = new QLabel{l.str().c_str()};

            layout->addWidget( _label, 0,0);

            _accept = new QPushButton;
            make_add_contact_small(*_accept);
            std::stringstream ss;
            ss << "Add `" << _intro.contact.name() << "'";
            _accept->setToolTip(ss.str().c_str());
            layout->addWidget(_accept, 0,1);

            _rm = make_x_button();
            std::stringstream ss2;
            ss2 << "Remove `" << _intro.contact.name() << "'";
            _rm->setToolTip(ss2.str().c_str());
            layout->addWidget(_rm, 0,2);

            std::stringstream m;
            m << "<i>" << _intro.message << "</i>";
            _message = new QLabel{m.str().c_str()};
            layout->addWidget( _message, 1,0);

            connect(_accept, SIGNAL(clicked()), this, SLOT(accept()));
            connect(_rm, SIGNAL(clicked()), this, SLOT(remove()));
        }

        void intro_info::remove()
        {
            INVARIANT(_service);
            INVARIANT(_accept);
            INVARIANT(_rm);
            INVARIANT(_label);
            auto from = _service->by_id(_intro.from_id);

            if(from)
            {
                std::stringstream msg;
                msg << "Are you sure you want to remove introduction for `" << _intro.contact.name() << "' from `" << from->name() << "'?";
                auto a = QMessageBox::warning(this, tr("Remove Introduction?"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
                if(a != QMessageBox::Yes) return;
            }

            _service->remove_introduction(_intro);
            _label->setEnabled(false);
            _rm->setEnabled(false);
            _accept->setEnabled(false);
            _message->setEnabled(false);
        }

        void intro_info::accept()
        {
            INVARIANT(_service);
            INVARIANT(_accept);
            INVARIANT(_rm);
            INVARIANT(_label);
            auto from = _service->by_id(_intro.from_id);

            if(from)
            {
                std::stringstream msg;
                msg << convert(tr("Are you sure you want to add contact `")) << _intro.contact.name() << convert(tr("' introduced by `")) << from->name() << "'?";
                auto a = QMessageBox::warning(this, tr("Add Contact?"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
                if(a != QMessageBox::Yes) return;
            }

            //add contact
            _service->confirm_contact({_intro.contact, _intro.greeter});
            _service->remove_introduction(_intro);
            _label->setEnabled(false);
            _rm->setEnabled(false);
            _accept->setEnabled(false);
            _message->setEnabled(false);
        }

        introduce_dialog::introduce_dialog(
                        const std::string& title, 
                        us::user_service_ptr s, 
                        QWidget* parent) : 
            QDialog{parent},
            _user_service{s}
        {
            REQUIRE(s);

            auto* layout = new QVBoxLayout{this};
            setLayout(layout);

            _contact_1 = new contact_select_widget{s};
            _contact_2 = new contact_select_widget{s};
            _message_1 = new QLineEdit;
            _message_2 = new QLineEdit;
            _message_label_1 = new QLabel;
            _message_label_2 = new QLabel;
            _introduce = new QPushButton;
            make_introduce(*_introduce);
            _introduce->setToolTip(tr("Introduce"));
            _introduce->setEnabled(false);
            _introduce->setStyleSheet("border: 0px; color: 'grey';");

            _cancel = new QPushButton;
            _cancel->setToolTip(tr("Cancel"));
            make_cancel(*_cancel);

            auto lw = new QWidget;
            auto ll = new QVBoxLayout{lw};

            ll->addWidget(new QLabel{tr("introduce")});
            ll->addWidget(_contact_1);
            ll->addWidget(_message_label_1);
            ll->addWidget(_message_1);

            auto rw = new QWidget;
            auto rl = new QVBoxLayout{rw};

            rl->addWidget(new QLabel{tr("to")});
            rl->addWidget(_contact_2);
            rl->addWidget(_message_label_2);
            rl->addWidget(_message_2);

            auto hw = new QWidget;
            auto hl = new QHBoxLayout{hw};
            hl->addWidget(lw);
            hl->addWidget(rw);

            layout->addWidget(hw);

            auto bw = new QWidget;
            auto bl = new QHBoxLayout{bw};
            bl->addWidget(_cancel);
            bl->addWidget(_introduce);

            layout->addWidget(bw);

            connect(_contact_1, SIGNAL(activated(int)), this, SLOT(contact_1_selected(int)));
            connect(_contact_2, SIGNAL(activated(int)), this, SLOT(contact_2_selected(int)));
            connect(_introduce, SIGNAL(clicked()), this, SLOT(introduce()));
            connect(_cancel, SIGNAL(clicked()), this, SLOT(cancel()));

            setWindowTitle(tr(title.c_str()));

            update_widgets();

            INVARIANT(_user_service);
            INVARIANT(_contact_1);
            INVARIANT(_contact_2);
            INVARIANT(_message_1);
            INVARIANT(_message_2);
            INVARIANT(_introduce);
            INVARIANT(_cancel);
        }

        void update_message_widgets(
                bool same,
                us::user_info_ptr c, 
                QLabel* label,
                QLineEdit* message)
        {
            REQUIRE(label);
            REQUIRE(message);
            if(!c || same)
            {
                label->setEnabled(false);
                message->setEnabled(false);
                return;
            }

            std::string l = convert(qApp->tr("tell ")) + c->name() + "...";
            label->setText(l.c_str());
            label->setEnabled(true);
            message->setEnabled(true);
        }

        void introduce_dialog::update_widgets()
        {
            INVARIANT(_user_service);
            INVARIANT(_contact_1);
            INVARIANT(_contact_2);
            INVARIANT(_message_1);
            INVARIANT(_message_2);
            INVARIANT(_message_label_1);
            INVARIANT(_message_label_2);
            INVARIANT(_introduce);

            auto c1 = _contact_1->selected_contact();
            auto c2 = _contact_2->selected_contact();

            bool same = false;
            _introduce->setEnabled(false);
            _introduce->setStyleSheet("border: 0px; color: 'grey';");
            if(c1 && c2)
            {
                same = c1->id() == c2->id();
                if(!same) 
                {
                    _introduce->setEnabled(true);
                    _introduce->setStyleSheet("border: 0px; color: 'green';");
                }
            }

            update_message_widgets(same, c1, _message_label_1, _message_1);
            update_message_widgets(same, c2, _message_label_2, _message_2);
        }

        void introduce_dialog::contact_1_selected(int)
        {
            update_widgets();
        }

        void introduce_dialog::contact_2_selected(int)
        {
            update_widgets();
        }

        void introduce_dialog::introduce()
        {
            INVARIANT(_user_service);
            INVARIANT(_user_service);
            INVARIANT(_contact_1);
            INVARIANT(_contact_2);
            INVARIANT(_message_1);
            INVARIANT(_message_2);

            auto c1 = _contact_1->selected_contact();
            auto c2 = _contact_2->selected_contact();
            if(!c1 || !c2)
            {
                update_widgets();
                return;
            }

            CHECK(c1);
            CHECK(c2);

            //validate that they want to introduce the two selected
            std::stringstream msg;
            msg << convert(tr("Are you sure you want to introduce `")) << c1->name() << convert(tr("' to `")) << c2->name() << "'?";
            auto a = QMessageBox::warning(this, tr("Introduce"), msg.str().c_str(), QMessageBox::Yes | QMessageBox::No);
            if(a != QMessageBox::Yes) return;

            std::string GREETER = ""; //TODO SET THIS

            us::contact_introduction i1{
                _user_service->user().info().id(),
                GREETER,
                convert(_message_1->text()),
                *c2
            };

            us::contact_introduction i2{
                _user_service->user().info().id(),
                GREETER,
                convert(_message_2->text()),
                *c1
            };

            _user_service->send_introduction(c1->id(), i1);
            _user_service->send_introduction(c2->id(), i2);
            accept();
        }

        void introduce_dialog::cancel()
        {
            reject();
        }

        QStringList greeter_list(us::user_service_ptr s)
        {
            REQUIRE(s);

            QStringList gs;
            for(const auto& g : s->user().greeters())
                gs << greet_address(g).c_str();
            return gs;
        }

        std::pair<std::string, bool> pick_greater(us::user_service_ptr s, QWidget* w)
        {
            REQUIRE(s);
            REQUIRE(w);

            auto total_greeters = s->user().greeters().size();
            std::string greeter;
            if(total_greeters == 1) greeter = greet_address(s->user().greeters().front());
            else if(total_greeters > 1)
            {
                auto gs = greeter_list(s);

                bool ok;
                auto g = QInputDialog::getItem(w, w->tr("Suggested Locator"),
                        w->tr("Choose a Locator:"), gs, 0, false, &ok);
                if (ok && !g.isEmpty())
                    greeter = convert(g);
                else return std::make_pair("", false);
            }

            return std::make_pair(greeter, true);
        }

        void email_identity(us::user_service_ptr s, QWidget* w)
        {
            REQUIRE(s);
            REQUIRE(w);

            const auto name = s->user().info().name();

            //user chooses greeter
            auto greeter = pick_greater(s, w);
            if(!greeter.second) return;

            std::stringstream url;

            url << "mailto:" << "?subject=" << name << " wants to connect with Fire★" 
                << "&body=" << name << " wants to connect with you using Fire★. \n\n" 
                                    << "You can download Fire★ at http://firestr.com\n\n"
                                    << "Copy and Paste the identity below into Fire★ and give them yours.\n\n";

            us::identity iden{ s->user().info(), greeter.first};
            url << us::create_identity(iden) << std::endl;

            QDesktopServices::openUrl(QUrl(url.str().c_str()));

        }

        add_contact_dialog::add_contact_dialog(QTabWidget* parent) : QDialog{parent}
        {
            auto layout = new QVBoxLayout{this};
            setLayout(layout);

            auto label = new QLabel{tr("Paste Identity Here")};
            _iden = new QTextEdit;

            _add = new QPushButton;
            make_add_contact(*_add);
            _add->setEnabled(false);
            _add->setStyleSheet("border: 0px; color: 'grey';");
            _add->setToolTip(tr("Add Contact"));

            _cancel = new QPushButton;
            _cancel->setToolTip(tr("Cancel"));
            make_cancel(*_cancel);

            layout->addWidget(label);
            layout->addWidget(_iden);

            auto bw = new QWidget;
            auto bl = new QHBoxLayout{bw};
            bl->addWidget(_cancel);
            bl->addWidget(_add);
            layout->addWidget(bw);

            connect(_iden, SIGNAL(textChanged()), this, SLOT(text_updated()));
            connect(_add, SIGNAL(clicked()), this, SLOT(add()));
            connect(_cancel, SIGNAL(clicked()), this, SLOT(cancel()));

            setWindowTitle(tr("Add Contact"));

            INVARIANT(_add);
            INVARIANT(_cancel);
            INVARIANT(_iden);
        }

        void add_contact_dialog::add()
        {
            accept();
        }

        void add_contact_dialog::cancel()
        {
            reject();
        }

        void add_contact_dialog::text_updated()
        { 
            INVARIANT(_add);
            INVARIANT(_iden);

            us::identity i;
            auto iden64 = iden();
            auto good = us::parse_identity(iden64, i);

            if(good) 
            {
                _add->setEnabled(true);
                _add->setStyleSheet("border: 0px; color: 'green';");
                _iden->setStyleSheet("QTextEdit { background-color: rgb(128, 255, 128) }");
            }
            else 
            {
                _add->setEnabled(false);
                _add->setStyleSheet("border: 0px; color: 'grey';");
                if(_iden->toPlainText().isEmpty()) 
                    _iden->setStyleSheet("QTextEdit { background-color: rgb(255, 255, 255) }");
                else 
                    _iden->setStyleSheet("QTextEdit { background-color: rgb(255, 128, 128) }");
            }
        }

        std::string add_contact_dialog::iden() const
        {
            INVARIANT(_iden);
            return convert(_iden->toPlainText());
        }

        show_identity_dialog::show_identity_dialog(user::user_service_ptr s, QWidget* parent) :
            QDialog{parent}, _s{s}
        {
            REQUIRE(s);

            auto layout = new QVBoxLayout{this};
            setLayout(layout);

            auto gs = greeter_list(_s);
            if(!gs.isEmpty()) _greeter = convert(gs.front());

            if(gs.size() > 1)
            {
                auto l1 = new QLabel{tr("Select a Locator")};
                _greeters = new QComboBox;
                _greeters->addItems(gs);
                layout->addWidget(l1);
                layout->addWidget(_greeters);
                connect(_greeters, SIGNAL(activated(int)), this, SLOT(greeter_selected(int)));
            }

            auto l2 = new QLabel{tr("Give This Identity to Others")};
            _iden = new QLabel;
            _iden->setWordWrap(true);
            _iden->setStyleSheet("color: 'grey';");

            auto l3 = new QLabel{tr("It is Copied to the Clipboard")};
            l3->setStyleSheet("color: 'red';");

            auto ok = new QPushButton;
            make_ok(*ok);

            layout->addWidget(l2);
            layout->addWidget(_iden);
            layout->addWidget(l3);
            layout->addWidget(ok);

            connect(ok, SIGNAL(clicked()), this, SLOT(ok()));

            setWindowTitle(tr("Your Identity"));
            update_identity();

            INVARIANT(_iden);
            INVARIANT(_s);
        }

        void show_identity_dialog::update_identity()
        {
            INVARIANT(_s);
            INVARIANT(_iden);

            us::identity i{ _s->user().info(), _greeter};
            auto iden = us::create_identity(i);

            _iden->setText(iden.c_str());

            auto clip = QApplication::clipboard();
            CHECK(clip);

            clip->setText(iden.c_str());
        }

        void show_identity_dialog::greeter_selected(int index)
        {
            INVARIANT(_greeters);

            _greeter = convert(_greeters->itemText(index));
            update_identity();
        }

        void show_identity_dialog::ok()
        {
            accept();
        }

        void show_identity_gui(user::user_service_ptr s, QWidget* p)
        {
            REQUIRE(s);
            REQUIRE(p);

            show_identity_dialog d{s, p};
            d.exec();
        }

        bool add_contact_gui(us::user_service_ptr us, const std::string& iden, QWidget* p)
        {
            REQUIRE_FALSE(iden.empty());
            REQUIRE(us);

            //load contact file
            us::identity i;

            bool added = false;

            if(us::parse_identity(iden, i)) 
            {
                //add greeter
                if(!i.greeter.empty())
                {
                    bool found = false;
                    for(const auto& s : us->user().greeters())
                    {
                        auto a = greet_address(s);
                        if(a == i.greeter) {found = true; break;}
                    }
                    if(found) i.greeter = "";
                }

                //add contact
                added = us->confirm_contact(i);
            }

            if(p)
            {
                std::stringstream ss;
                if(added)
                {
                    ss << "`" << i.contact.name() << "' has been added";
                    QMessageBox::information(p, p->tr("Contact Added"), ss.str().c_str());
                }
                else
                {
                    ss << "There is something wrong with the identity." << std::endl;
                    QMessageBox::critical(p, p->tr("Error Adding Contact"), ss.str().c_str());
                }
            }
            
            return added;
        }

        bool add_contact_gui(us::user_service_ptr s, QWidget* p)
        {
            REQUIRE(s);
            REQUIRE(p);

            //get file name to load
            add_contact_dialog ac;
            ac.exec();
            
            if(ac.result() == QDialog::Rejected) return false;

            auto iden = ac.iden();
            return add_contact_gui(s, iden, p);
        }

    }
}
