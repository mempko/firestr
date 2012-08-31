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

#include "gui/test_message.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"

namespace ms = fire::messages;
namespace us = fire::user;

namespace fire
{
    namespace gui
    {
        test_message::test_message(ms::sender_ptr sender) :
            _m{},
            _sender{sender}
        {
            REQUIRE(sender);
            INVARIANT(root());
            INVARIANT(layout());

            init_send();

            INVARIANT(_sender);
        }
        test_message::test_message(const ms::test_message& m, ms::sender_ptr sender) : 
            _m{m},
            _sender{sender}
        {
            REQUIRE(sender);
            INVARIANT(root());
            INVARIANT(layout());

            init_reply();

            INVARIANT(_sender);
        }

        void test_message::init_send() 
        {
            INVARIANT(root());
            INVARIANT(layout());

            //user select
            _users = new QComboBox;
            for(auto p : _sender->user_service()->user().contacts())
                _users->addItem(p->name().c_str(), p->id().c_str());
            layout()->addWidget(_users, 0, 0);

            //text edit
            _etext = new QLineEdit;
            layout()->addWidget(_etext, 1, 0);

            //send button
            _send = new QPushButton{"send"};
            connect(_send, SIGNAL(clicked()), this, SLOT(send_message()));
            layout()->addWidget(_send, 1, 1);

            INVARIANT(_users);
            INVARIANT(_etext);
            INVARIANT(_send);
        }

        void test_message::init_reply() 
        {
            INVARIANT(root());
            INVARIANT(layout());

            //message
            auto contact = _sender->user_service()->user().contact_by_id(_m.from_id());

            std::string user_name = contact ? contact->name() : "spy";
            user_name = "<b>" + user_name + "</b>";
            
            QLabel* user = new QLabel{user_name.c_str()};
            layout()->addWidget(user, 0,0);

            QLabel* text = new QLabel{_m.text().c_str()};
            layout()->addWidget(text, 1,0, 1,1);

            //text edit
            _etext = new QLineEdit;
            layout()->addWidget(_etext, 2, 0);

            //reply button
            _reply = new QPushButton{"reply"};
            connect(_reply, SIGNAL(clicked()), this, SLOT(send_reply()));
            layout()->addWidget(_reply, 2,1);

            INVARIANT(_etext);
            INVARIANT(_reply);
        }

        void test_message::send_message()
        {
            INVARIANT(_sender);
            INVARIANT(_users);
            INVARIANT(_etext);
            INVARIANT(_send);

            size_t i = _users->currentIndex();
            auto id = convert(_users->itemData(i).toString());

            auto contact = _sender->user_service()->user().contact_by_id(id);
            if(!contact) return;

            _users->setEnabled(false);
            _etext->setEnabled(false);
            _send->hide();

            auto text = convert(_etext->text());

            ms::test_message tm(text);
            _sender->send(id, tm); 
        }

        void test_message::send_reply()
        {
            INVARIANT(_sender);
            INVARIANT(_etext);
            INVARIANT(_reply);

            auto id = _m.from_id();
            auto contact = _sender->user_service()->user().contact_by_id(id);
            if(!contact) return;

            _etext->setEnabled(false);
            _reply->hide();

            auto text = convert(_etext->text());
            ms::test_message tm(text);
            _sender->send(id, tm); 
        }
    }
}

