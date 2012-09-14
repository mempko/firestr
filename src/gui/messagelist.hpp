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
#include "gui/app/app_service.hpp"
#include "session/session.hpp"
#include "messages/new_app.hpp"

namespace fire
{
    namespace gui
    {
        class message_list : public list
        {
            Q_OBJECT
            public:
                message_list(
                        app::app_service_ptr,
                        session::session_ptr);

            public:
                session::session_ptr session();
                app::app_service_ptr app_service();

            public slots:
                void add_new_app(const messages::new_app&); 
                void add(message*);
                void add(QWidget*);
                void scroll_to_bottom(int min, int max);

            private:
                QScrollBar* _scrollbar;
                session::session_ptr _session;
                app::app_service_ptr _app_service;
        };
    }
}

#endif
