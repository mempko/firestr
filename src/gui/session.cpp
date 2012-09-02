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

#include <QtGui>

namespace fire
{
    namespace gui
    {
        session_widget::session_widget(session::session_ptr session) :
            _session{session},
            _messages{new message_list{session}}
        {
            REQUIRE(session);

            _layout = new QGridLayout;
            _layout->addWidget(_messages);

            setLayout(_layout);
            _layout->setContentsMargins(0,0,0,0);

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
    }
}

