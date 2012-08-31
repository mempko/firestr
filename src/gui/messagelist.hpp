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

#ifndef FIRESTR_GUI_MESSAGELIST_H
#define FIRESTR_GUI_MESSAGELIST_H

#include <QScrollArea>

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"

namespace fire
{
    namespace gui
    {
        class message_list : public list
        {
            Q_OBJECT
            public:
                message_list(session::session_ptr);

            public:
                session::session_ptr session();

            public slots:
                void add(message* m);
                void check_mail(); 
                void scroll_to_bottom(int min, int max);

            private:
                QScrollBar* _scrollbar;
                session::session_ptr _session;
        };
    }
}

#endif
