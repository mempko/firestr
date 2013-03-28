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

#ifndef FIRESTR_DEBUGWIN_H
#define FIRESTR_DEBUGWIN_H

#include "user/userservice.hpp"
#include "session/session_service.hpp"
#include "message/postoffice.hpp"

#include "gui/list.hpp"

#include <fstream>
#include <set>

#include <QDialog>
#include <QTextEdit>
#include <QDateTime>
#include <QGraphicsView>
#include <QGridLayout>
#include <QLabel>


namespace fire
{
    namespace gui
    {
        class mailbox_debug : public QWidget
        {
            Q_OBJECT
            public:
                mailbox_debug(fire::message::mailbox_wptr m);

            public slots:
                void update_graph();

            private:
                QGraphicsView* _in_graph;
                QGraphicsView* _out_graph;
                size_t _in_max;
                size_t _out_max;
                size_t _x;
                size_t _prev_in_y;
                size_t _prev_out_y;
                QLabel* _name;
                fire::message::mailbox_wptr _mailbox;
        };

        using added_mailboxes = std::set<std::string>;

        class debug_win : public QDialog
        {
            Q_OBJECT
            public:
                debug_win(
                        fire::message::post_office_ptr,
                        user::user_service_ptr, 
                        session::session_service_ptr, 
                        QWidget* parent = nullptr);

            public slots:
                void update_log();
                void update_mailboxes();

            private:
                QTextEdit* _log;
                list* _mailboxes;
                added_mailboxes _added_mailboxes;

                size_t _total_mailboxes;
                QDateTime _log_last_modified;
                std::streampos _log_last_file_pos;
                fire::message::post_office_ptr _post;
                user::user_service_ptr _user_service;
                session::session_service_ptr _session_service;
        };
    }
}

#endif
