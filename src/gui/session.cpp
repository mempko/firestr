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
            const size_t CW_WIDTH = 50;//in milliseconds
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
            _add_contact = new QPushButton{"add"};
            connect(_add_contact, SIGNAL(clicked()), this, SLOT(add_contact()));

            update_contact_select();

            _contacts = new contact_list{_session_service->user_service(), _session->contacts()};

            auto* cw = new QWidget;
            auto* cl = new QGridLayout;
            cw->setLayout(cl);
            cl->addWidget(_contact_select, 0,0);
            cl->addWidget(_add_contact, 0, 1);
            cl->addWidget(_contacts, 1, 0, 1, 2);
            cw->resize(CW_WIDTH, cw->height());

            _splitter = new QSplitter{Qt::Horizontal};
            _splitter->addWidget(cw);
            _splitter->addWidget(_messages);
            _splitter->setStretchFactor(0, 0);
            _splitter->setStretchFactor(1, 2);
           
            _layout->addWidget(_splitter);

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
        
        void session_widget::update_contact_select()
        {
            INVARIANT(_contact_select);
            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_add_contact);

            _contact_select->clear();
            for(auto p : _session_service->user_service()->user().contacts().list())
            {
                CHECK(p);
                if(_session->contacts().by_id(p->id())) continue;

                _contact_select->addItem(p->name().c_str(), p->id().c_str());
            }

            bool enabled = _contact_select->count() > 0;

            _contact_select->setEnabled(enabled);
            _add_contact->setEnabled(enabled);
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

            update_contact_select();
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
            update_contact_select();
        }

        void session_widget::name(const QString& s)
        {
            _name = s;
        }

        QString session_widget::name() const
        {
            return _name;
        }
    }
}

