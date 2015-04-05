/*
 * Copyright (C) 2014  Maxim Noah Khailo
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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * Botan library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than Botan. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#ifndef FIRESTR_DEBUGWIN_H
#define FIRESTR_DEBUGWIN_H

#include "user/user_service.hpp"
#include "conversation/conversation_service.hpp"
#include "message/post_office.hpp"
#include "network/udp_queue.hpp"

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
        class queue_debug : public QWidget
        {
            Q_OBJECT
            public:
                queue_debug(fire::message::mailbox_wptr m);
                queue_debug(const std::string& name, fire::message::mailbox_stats&);
                ~queue_debug();

            public slots:
                void update_graph();

            private:
                void init_gui();

            private:
                QGraphicsView* _in_graph;
                QGraphicsView* _out_graph;
                int _in_max = 1;
                int _out_max = 1;
                int _x = 0;
                int _prev_in_push = 0;
                int _prev_out_push = 0;
                int _prev_in_pop = 0;
                int _prev_out_pop = 0;
                QLabel* _name;
                std::string _name_s;
                fire::message::mailbox_wptr _mailbox;
                fire::message::mailbox_stats* _mailbox_stats;
        };

        using added_mailboxes = std::set<std::string>;

        class debug_win : public QDialog
        {
            Q_OBJECT
            public:
                debug_win(
                        fire::message::post_office_ptr,
                        user::user_service_ptr, 
                        conversation::conversation_service_ptr, 
                        const network::udp_stats&,
                        QWidget* parent = nullptr);

            public slots:
                void update_log();
                void update_mailboxes();
                void update_udp_stats();

            private slots:
                void closeEvent(QCloseEvent*);

            private:
                void save_state();
                void restore_state();

            private:
                QTextEdit* _log;
                list* _mailboxes;
                QLabel* _udp_stat_text;

                added_mailboxes _added_mailboxes;

                size_t _total_mailboxes = 0;
                QDateTime _log_last_modified;
                std::streampos _log_last_file_pos = 0;
                fire::message::post_office_ptr _post;
                user::user_service_ptr _user_service;
                conversation::conversation_service_ptr _conversation_service;

                //stats
                const network::udp_stats& _udp_stats;
                network::udp_stats _prev_udp_stats;
        };
    }
}

#endif
