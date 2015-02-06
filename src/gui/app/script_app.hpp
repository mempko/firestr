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

#ifndef FIRESTR_APP_SCRIPT_APP_H
#define FIRESTR_APP_SCRIPT_APP_H

#include "gui/app/app.hpp"
#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "gui/app/generic_app.hpp"
#include "gui/list.hpp"
#include "gui/lua/api.hpp"
#include "gui/lua/backend_client.hpp"
#include "gui/qtw/frontend_client.hpp"

#include "conversation/conversation_service.hpp"

#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>

#include <string>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class script_app : public generic_app
            {
                Q_OBJECT

                public:
                    script_app(
                            app_ptr, 
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr);
                    script_app(
                            const std::string& from_id, 
                            const std::string& id, 
                            app_ptr, 
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr);
                    ~script_app();

                public:
                    const std::string& id() const;
                    const std::string& type() const;
                    fire::message::mailbox_ptr mail();

                public slots:
                    void clone_app();
                    void got_alert();

                private:
                    void setup_decorations();
                    void init();

                private:
                    std::string _from_id;
                    std::string _id;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;

                    app_ptr _app;
                    app_service_ptr _app_service;
                    app_reaper_ptr _app_reaper;

                private:
                    qtw::qt_frontend_client_ptr _front;
                    lua::backend_client_ptr _back;
                    lua::lua_api_ptr _api;
                    QWidget* _canvas = nullptr;
                    QGridLayout* _canvas_layout = nullptr;
                    QPushButton* _clone = nullptr;
            };

            extern const std::string SCRIPT_APP;

        }
    }
}

#endif

