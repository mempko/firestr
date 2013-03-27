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

#include <fstream>

#include <QDialog>
#include <QTextEdit>
#include <QDateTime>


namespace fire
{
    namespace gui
    {
        class debug_win : public QDialog
        {
            Q_OBJECT
            public:
                debug_win(
                        user::user_service_ptr, 
                        session::session_service_ptr, 
                        QWidget* parent = nullptr);

            public slots:
                void update_log();

            protected:

            protected:
                QTextEdit* _log;
                QDateTime _log_last_modified;
                std::streampos _log_last_file_pos;
                user::user_service_ptr _user_service;
                session::session_service_ptr _session_service;
        };
    }
}

#endif
