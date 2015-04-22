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

#ifndef FIRESTR_GUI_MESSAGELIST_H
#define FIRESTR_GUI_MESSAGELIST_H

#include <QMdiArea>

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "gui/app/generic_app.hpp"
#include "conversation/conversation_service.hpp"
#include "messages/new_app.hpp"

namespace fire
{
    namespace gui
    {
        //joins and app with the generic_app widget.
        //app here may be null but widget should not be.
        struct app_pair
        {
            app::app_ptr app;
            app::generic_app* widget;
        };

        using app_map = std::unordered_map<std::string, app_pair>; 

        class app_area : public QMdiArea
        {
            Q_OBJECT
            public:
                app_area(
                        app::app_service_ptr,
                        app::app_reaper_ptr,
                        conversation::conversation_service_ptr,
                        conversation::conversation_ptr);

            public:
                conversation::conversation_ptr conversation();
                app::app_service_ptr app_service();
                const app_map& apps() const;

            public:
                void add_chat_app();
                void add_app_editor(const std::string& id);
                void add_script_app(const std::string& id);
                void add(app::generic_app*);
                bool add_new_app(const messages::new_app&); 

            public slots:
                void clear_alerts();
                void sub_window_activated(QMdiSubWindow*);

            private slots:
                void handle_resize_hack();

            private:
                void add(app::generic_app*, app::app_ptr, const std::string& id);

            private:
                conversation::conversation_service_ptr _conversation_service;
                conversation::conversation_ptr _conversation;
                app::app_service_ptr _app_service;
                app::app_reaper_ptr _app_reaper;
                app_map _apps;
        };
    }
}

#endif
