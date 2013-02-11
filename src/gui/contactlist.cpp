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

#include "gui/contactlist.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"

#include <QtGui>
#include <QGridLayout>
#include <QLabel>

namespace u = fire::util;
namespace usr = fire::user;
                
namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 200;//in milliseconds
        }

        std::string online_text(
                user::user_info_ptr c, 
                user::user_service_ptr s)
        {
            REQUIRE(c);
            REQUIRE(s);
            return s->contact_available(c->id()) ? "online" : "offline";
        }

        user_info::user_info(
                user::user_info_ptr p, 
                user::user_service_ptr s,
                bool accept_reject,
                bool compact) :
            _contact{p},
            _service{s}
        {
            REQUIRE(p)
            REQUIRE(s)

            auto* layout = new QGridLayout{this};
            setLayout(layout);

            if(compact)
            {
                layout->addWidget( new QLabel{p->name().c_str()}, 0,0);
                if(!accept_reject)
                {
                    _online = new QLabel{online_text(p, _service).c_str()};
                    _online->setToolTip(p->address().c_str());
                    layout->addWidget(_online, 0,1);
                }
            }
            else
            {
                layout->addWidget( new QLabel{"Name:"}, 0,0);
                layout->addWidget( new QLabel{p->name().c_str()}, 0,1);
                layout->addWidget( new QLabel{"Address:"}, 1,0);
                layout->addWidget( new QLabel{p->address().c_str()}, 1,1);
                if(!accept_reject)
                {
                    _online = new QLabel{online_text(p, _service).c_str()};
                    layout->addWidget(_online, 2,0);
                }
            }

            if(accept_reject)
            {
                //create add button
                _accept = new QPushButton("accept");
                _reject = new QPushButton("reject");
                layout->addWidget(_accept, 3,0); 
                layout->addWidget(_reject, 3,1); 

                connect(_accept, SIGNAL(clicked()), this, SLOT(accept()));
                connect(_reject, SIGNAL(clicked()), this, SLOT(reject()));
            }

            layout->setContentsMargins(2,2,2,2);
            ENSURE(_contact);
        }

        void user_info::update()
        {
            INVARIANT(_service);
            INVARIANT(_contact);
            INVARIANT(_online);

            _online->setText(online_text(_contact, _service).c_str());
        }

        void user_info::accept()
        {
            REQUIRE(_service);
            REQUIRE(_accept);
            REQUIRE(_reject);
            INVARIANT(_contact);

            _service->send_confirmation(_contact->id());

            _accept->setText("accepted");
            _accept->setEnabled(false);
            _reject->setEnabled(false);
            _reject->hide();
        }

        void user_info::reject()
        {
            REQUIRE(_service);
            REQUIRE(_accept);
            REQUIRE(_reject);
            INVARIANT(_contact);

            _service->send_rejection(_contact->id());

            _reject->setText("rejected");
            _reject->setEnabled(false);
            _accept->setEnabled(false);
            _accept->hide();
        }

        contact_list_dialog::contact_list_dialog(
                const std::string& title, 
                user::user_service_ptr service,
                bool add_on_start) :
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

            auto* greeters_tab = new QWidget;
            auto* greeters_layout = new QGridLayout{greeters_tab};
            tabs->addTab(contacts_tab, "contacts");
            tabs->addTab(greeters_tab, "greeters");

            init_contacts_tab(contacts_tab, contacts_layout, add_on_start);
            init_greeters_tab(greeters_tab, greeters_layout);

            setWindowTitle(tr(title.c_str()));

            //create greeter tab
            INVARIANT(_list);
        }

        void contact_list_dialog::init_contacts_tab(QWidget* tab, QGridLayout* layout, bool add_on_start)
        {
            REQUIRE(tab);
            REQUIRE(layout);
            INVARIANT(_service);

            //create contact list
            _list = new list;
            layout->addWidget(_list, 0, 0, 2, 3);

            update_contacts();

            //create add button
            auto* add_new = new QPushButton("add");
            layout->addWidget(add_new, 2,0); 
            connect(add_new, SIGNAL(clicked()), this, SLOT(new_contact()));

            //create id label
            std::string id = _service->user().info().id(); 
            auto* id_label = new QLabel("your id");
            auto* id_txt = new QLineEdit(id.c_str());
            id_txt->setMinimumWidth(id.size() * 8);
            id_txt->setReadOnly(true);
            id_txt->setFrame(false);
            layout->addWidget(id_label, 3,0); 
            layout->addWidget(id_txt, 3,1,1,2); 

            std::string addr = _service->in_host() + ":" + _service->in_port();
            auto* addr_label = new QLabel("your address");
            auto* addr_txt = new QLineEdit(addr.c_str());
            addr_txt->setMinimumWidth(addr.size() * 8);
            addr_txt->setReadOnly(true);
            addr_txt->setFrame(false);
            layout->addWidget(addr_label, 4,0); 
            layout->addWidget(addr_txt, 4,1,1,2); 

            //setup updated timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update()));
            t->start(TIMER_SLEEP);

            if(add_on_start) new_contact();
        }

        void contact_list_dialog::init_greeters_tab(QWidget* tab, QGridLayout* layout)
        {
            REQUIRE(tab);
            REQUIRE(layout);
            INVARIANT(_service);
            auto gl = new greeter_list{_service};

            layout->addWidget(gl, 0,0);
            auto* add_new = new QPushButton("add");
            layout->addWidget(add_new, 1,0); 
            connect(add_new, SIGNAL(clicked()), gl, SLOT(add_greeter()));
        }

        void contact_list_dialog::update_contacts()
        {
            INVARIANT(_list);
            INVARIANT(_service);

            _list->clear();

            for(auto u : _service->user().contacts().list())
                _list->add(new user_info{u, _service, false, true});

            auto pending = _service->pending_requests();

            for(auto r : pending)
                _list->add(new user_info{r.second.from, _service, true});

            _prev_requests = pending.size();
            _prev_contacts = _service->user().contacts().size();
        }

        void contact_list_dialog::new_contact()
        {
            INVARIANT(_service);

            bool ok = false;
            std::string address = "<host>:6060";

            QString r = QInputDialog::getText(
                    0, 
                    "Add New Contact",
                    "Contact Address or ID",
                    QLineEdit::Normal, address.c_str(), &ok);

            if(ok && !r.isEmpty()) address = convert(r);
            else return;

            _service->attempt_to_add_contact(address);
        }

        void contact_list_dialog::update()
        {
            size_t pending_requests = _service->pending_requests().size();
            size_t contacts = _service->user().contacts().size();
            if(pending_requests == _prev_requests && contacts == _prev_contacts) 
                return;

            update_contacts();
            _prev_requests = pending_requests;
            _prev_contacts = contacts;
        }

        contact_list::contact_list(user::user_service_ptr service, const user::contact_list& contacts) :
            _service{service}
        {
            REQUIRE(service);
            for(auto u : contacts.list())
            {
                CHECK(u);
                add_contact(u);
            }

            INVARIANT(_service);
        }

        void contact_list::add_contact(user::user_info_ptr u)
        {
            REQUIRE(u);
            INVARIANT(_service);
            if(_contacts.by_id(u->id())) return;

            auto ui = new user_info{u, _service, false, true};
            add(ui);
            _contacts.add(u);
            _contact_widgets.push_back(ui);
        }

        void contact_list::update(const user::contact_list& contacts)
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

        void contact_list::update_status()
        {
            for(auto p : _contact_widgets)
            {
                CHECK(p);
                p->update();
            }
        }

        greeter_list::greeter_list(user::user_service_ptr service) :
            _service{service}

        {
            REQUIRE(service);
            INVARIANT(_service);
            for(const auto& s : _service->user().greeters())
                add_greeter(s);
        }
                
        void greeter_list::add_greeter(const user::greet_server& s)
        {
            add(new greeter_info{_service, s});
        }

        void greeter_list::add_greeter()
        {
            bool ok = false;
            bool error = false;
            std::string address = "<host>:7070";

            do
            {
                try
                {
                    QString r = QInputDialog::getText(
                            0, 
                            (error ? "Error, try again" : "Add New Greeter"),
                            "Greeter Address",
                            QLineEdit::Normal, address.c_str(), &ok);

                    if(ok && !r.isEmpty()) address = convert(r);
                    else return;

                    error = false;
                }
                catch(...)
                {
                    error = true;
                }
            }
            while(error);

            _service->add_greeter(address);
            CHECK_FALSE(_service->user().greeters().empty());
            add_greeter(_service->user().greeters().back());
        }

        void greeter_list::remove_greeter()
        {

        }

        greeter_info::greeter_info(user::user_service_ptr service, const user::greet_server& s) :
            _server{s}, _service{service}
        {
            INVARIANT(_service);

            auto* layout = new QGridLayout{this};
            setLayout(layout);
            auto label = _server.host() + ":" + _server.port();
            layout->addWidget( new QLabel{label.c_str()}, 0,0);
        }

    }
}
