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
 */

#ifndef FIRESTR_GUI_APP_SERVICE_H
#define FIRESTR_GUI_APP_SERVICE_H

#include "service/service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#include "gui/app/app.hpp"
#include "util/thread.hpp"

#include <string>
#include <map>
#include <set>
#include <memory>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            using app_metadata_map = std::map<std::string, app_metadata>;

            class app_service : public service::service 
            {
                public:
                    app_service(
                            user::user_service_ptr,
                            fire::message::mailbox_ptr event = nullptr);

                public:
                    app_metadata_map available_apps() const;
                    app_ptr load_app(const std::string& id) const;
                    bool save_app(const app&);
                    bool clone_app(app&);

                public:
                    user::user_service_ptr user_service();

                protected:
                    virtual void message_received(const fire::message::message&);

                private:
                    void fire_apps_updated_event();

                private:
                    void load_apps();

                private:
                    std::string _app_home;
                    app_metadata_map _app_metadata;
                    mutable std::mutex _mutex;

                    user::user_service_ptr _user_service;
                    messages::sender_ptr _sender;
            };

            using app_service_ptr = std::shared_ptr<app_service>;
            using app_service_wptr = std::weak_ptr<app_service>;

            //events
            namespace event
            {
                extern const std::string APPS_UPDATED;
                struct apps_updated {};

                fire::message::message convert(const apps_updated&);
                void convert(const fire::message::message&, apps_updated&);
            }
        }
    }
}

#endif

