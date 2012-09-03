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
namespace s = fire::session;

namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t PADDING = 20;
        }

        test_message::test_message(s::session_ptr session) :
            _m{},
            _session{session}
        {
            REQUIRE(session);
            INVARIANT(root());
            INVARIANT(layout());

            init_send();

            setMinimumHeight(layout()->sizeHint().height() + PADDING);

            INVARIANT(_session);
        }

        test_message::test_message(const ms::test_message& m, s::session_ptr session) : 
            _m{m},
            _session{session}
        {
            REQUIRE(session);
            INVARIANT(root());
            INVARIANT(layout());

            init_reply();

            setMinimumHeight(layout()->sizeHint().height() + PADDING);

            INVARIANT(_session);
        }

        void test_message::init_send() 
        {
            INVARIANT(root());
            INVARIANT(layout());

            auto contacts = _session->contacts();
            if(contacts.empty())
            {
                auto l = new QLabel{"no contacts select in session"};
                auto s = new QPushButton{"send"};
                s->setEnabled(false);
                layout()->addWidget(l, 0, 0);
                layout()->addWidget(s, 0, 1);
                return;
            }

            //text edit
            _etext = new QLineEdit;
            layout()->addWidget(_etext, 1, 0);

            //send button
            _send = new QPushButton{"send"};
            layout()->addWidget(_send, 1, 1);
            
            connect(_etext, SIGNAL(returnPressed()), this, SLOT(send_message()));
            connect(_send, SIGNAL(clicked()), this, SLOT(send_message()));

            INVARIANT(_etext);
            INVARIANT(_send);
        }

        void test_message::init_reply() 
        {
            INVARIANT(root());
            INVARIANT(layout());

            //message
            auto contact = _session->contacts().by_id(_m.from_id());

            std::string user_name = contact ? contact->name() : "spy";
            user_name = "<b>" + user_name + "</b>";
            
            auto* user = new QLabel{user_name.c_str()};
            layout()->addWidget(user, 0,0);

            auto* text = new QLabel{_m.text().c_str()};
            layout()->addWidget(text, 1,0, 1,1);

            //text edit
            _etext = new QLineEdit;
            layout()->addWidget(_etext, 2, 0);

            //reply button
            _reply = new QPushButton{"reply"};
            layout()->addWidget(_reply, 2,1);

            connect(_etext, SIGNAL(returnPressed()), this, SLOT(send_reply()));
            connect(_reply, SIGNAL(clicked()), this, SLOT(send_reply()));

            INVARIANT(_etext);
            INVARIANT(_reply);
        }

        void test_message::send_message()
        {
            INVARIANT(_etext);
            INVARIANT(_send);
            INVARIANT(_session);

            _etext->setEnabled(false);
            _send->hide();

            auto text = convert(_etext->text());

            ms::test_message tm(text);

            for(auto c : _session->contacts().list())
            {
                CHECK(c);
                _session->sender()->send(c->id(), tm); 
            }
        }

        void test_message::send_reply()
        {
            INVARIANT(_etext);
            INVARIANT(_reply);
            INVARIANT(_session);

            auto id = _m.from_id();
            auto contact = _session->contacts().by_id(id);
            if(!contact) return;

            _etext->setEnabled(false);
            _reply->hide();

            auto text = convert(_etext->text());
            ms::test_message tm(text);

            for(auto c : _session->contacts().list())
            {
                CHECK(c);
                _session->sender()->send(c->id(), tm); 
            }
        }
    }
}

