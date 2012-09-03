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
                layout->addWidget( new QLabel{p->address().c_str()}, 0,1);
            }
            else
            {
                layout->addWidget( new QLabel{"Name:"}, 0,0);
                layout->addWidget( new QLabel{p->name().c_str()}, 0,1);
                layout->addWidget( new QLabel{"Address:"}, 1,0);
                layout->addWidget( new QLabel{p->address().c_str()}, 1,1);
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

        contact_list_dialog::contact_list_dialog(const std::string& title, user::user_service_ptr service) :
            _service{service}
        {
            REQUIRE(service);
            
            auto* layout = new QGridLayout{this};
            setLayout(layout);

            //create list
            _list = new list;
            layout->addWidget(_list, 0, 0);

            update_contacts();

            //create add button
            auto* add_new = new QPushButton("add");
            layout->addWidget(add_new, 1,0); 

            connect(add_new, SIGNAL(clicked()), this, SLOT(new_contact()));

            //setup updated timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update()));
            t->start(TIMER_SLEEP);

            setWindowTitle(tr(title.c_str()));

            INVARIANT(_list);
        }

        void contact_list_dialog::update_contacts()
        {
            INVARIANT(_list);
            INVARIANT(_service);

            _list->clear();

            for(auto u : _service->user().contacts())
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
            std::string address = "<host>:<ip>";

            QString r = QInputDialog::getText(
                    0, 
                    "Add New Contact",
                    "Contact Address",
                    QLineEdit::Normal, address.c_str(), &ok);

            if(ok && !r.isEmpty()) address = convert(r);
            else return;

            _service->attempt_to_add_user(address);
        }

        void contact_list_dialog::update()
        {
            size_t pending_requests = _service->pending_requests().size();
            size_t contacts = _service->user().contacts().size();
            if(pending_requests == _prev_requests && contacts == _prev_contacts) return;

            update_contacts();
            _prev_requests = pending_requests;
            _prev_contacts = contacts;
        }

        contact_list::contact_list(user::user_service_ptr service, const user::contact_list& contacts) :
            _service{service}
        {
            REQUIRE(service);
            for(auto u : contacts)
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

            add(new user_info{u, _service, false, true});
            _contacts.add(u);
        }

        void contact_list::update(const user::contact_list& contacts)
        {
            clear();
            _contacts.clear();
            for(auto c : contacts)
            {
                CHECK(c);
                add_contact(c);
            }

            ENSURE_LESS_EQUAL(_contacts.size(), contacts.size());
        }
    }
}
