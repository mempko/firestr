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


#include "gui/session.hpp"
#include "gui/util.hpp"

#include <QtGui>

namespace fire
{
    namespace gui
    {
        namespace 
        {
            const size_t TIMER_SLEEP = 200;//in milliseconds
        }

        session_widget::session_widget(
                session::session_service_ptr session_service,
                session::session_ptr session) :
            _session_service{session_service},
            _session{session},
            _messages{new message_list{session}}
        {
            REQUIRE(session_service);
            REQUIRE(session);

            _layout = new QGridLayout;

            _contact_select = new QComboBox;
            for(auto p : _session_service->user_service()->user().contacts())
                _contact_select->addItem(p->name().c_str(), p->id().c_str());

            _add_contact = new QPushButton{"add"};
            connect(_add_contact, SIGNAL(clicked()), this, SLOT(add_contact()));
            _contacts = new contact_list{_session_service->user_service(), _session->contacts()};

           
            _layout->addWidget(_contact_select, 0,0);
            _layout->addWidget(_add_contact, 0, 1);
            _layout->addWidget(_contacts, 1, 0, 1, 2);
            _layout->addWidget(_messages, 2, 0, 1, 2);

            _layout->setRowStretch(0, 0);
            _layout->setRowStretch(1, 0);
            _layout->setRowStretch(2, 2);

            setLayout(_layout);
            _layout->setContentsMargins(0,0,0,0);

            //setup updated timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update()));
            t->start(TIMER_SLEEP);

            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_messages);
            INVARIANT(_layout);
        }

        void session_widget::add(message* m)
        {
            INVARIANT(_messages);
            _messages->add(m);
        }

        session::session_ptr session_widget::session()
        {
            ENSURE(_session);
            return _session;
        }

        void session_widget::add_contact()
        {
            INVARIANT(_contact_select);
            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_contacts);

            size_t i = _contact_select->currentIndex();
            auto id = convert(_contact_select->itemData(i).toString());

            auto contact = _session_service->user_service()->user().contacts().by_id(id);
            if(!contact) return;

            _session_service->add_contact_to_session(contact, _session);
            _contacts->add_contact(contact);
        }

        void session_widget::update()
        {
            size_t contacts = _session->contacts().size();
            if(contacts == _prev_contacts) return;

            update_contacts();
            _prev_contacts = _session->contacts().size();

        }

        void session_widget::update_contacts()
        {
            _contacts->update(_session->contacts());
        }

    }
}

