/*
 * Copyright (C) 2013  Maxim Noah Khailo
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

#ifndef FIRESTR_GUI_SESSION_H
#define FIRESTR_GUI_SESSION_H

#include "gui/messagelist.hpp"
#include "gui/contactlist.hpp"
#include "gui/app/app_service.hpp"
#include "session/session.hpp"
#include "session/session_service.hpp"

#include <QWidget>
#include <QGridLayout>
#include <QComboBox>
#include <QSplitter>

namespace fire
{
    namespace gui
    {
        class session_widget : public QWidget
        {
            Q_OBJECT
            public:
                session_widget(
                        session::session_service_ptr, 
                        session::session_ptr,
                        app::app_service_ptr);

            public:
                session::session_ptr session();

                void name(const QString&);
                QString name() const;

            public slots:
                void check_mail(); 
                void add(message*);
                void add(QWidget*);
                void add_contact();
                void update_contacts();
                void update_contact_select();

            private:
                QGridLayout* _layout;
                QComboBox* _contact_select;
                QPushButton* _add_contact;
                QSplitter* _splitter;
                QString _name;
                message_list* _messages;
                contact_list* _contacts;
                session::session_ptr _session;
                session::session_service_ptr _session_service;
                app::app_service_ptr _app_service;
        };
    }
}

#endif
