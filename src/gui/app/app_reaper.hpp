/*
 * Copyright (C) 2015  Maxim Noah Khailo
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

#ifndef FIRESTR_GUI_APP_REAPER_H
#define FIRESTR_GUI_APP_REAPER_H

#include "gui/lua/backend_client.hpp"
#include "gui/qtw/frontend_client.hpp"

namespace fire
{
    namespace gui
    {
        namespace app
        {
            struct closed_app
            {
                QWidget* hidden;
                std::string name;
                std::string id;
                qtw::qt_frontend_client_ptr front;
                lua::backend_client_ptr back;
            };

            using closed_queue = util::queue<closed_app>;

            class app_reaper : QObject
            {
                Q_OBJECT
                public:
                    app_reaper(QWidget* parent);
                    ~app_reaper();

                    void reap(closed_app&);
                    void stop();
                public:
                    void emit_cleanup(closed_app);

                signals:
                    void got_cleanup(closed_app);

                private slots:
                    void do_cleanup(closed_app);

                private:
                    
                    friend void reap_thread(app_reaper*);
                    closed_queue _closed;
                    util::thread_uptr _thread;
                    QWidget* _hidden;
            };

            using app_reaper_ptr = std::shared_ptr<app_reaper>;
        }
    }
}
#endif
