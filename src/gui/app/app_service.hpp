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
                    app_ptr create_new_app() const;
                    app_ptr create_app(const fire::message::message&) const;
                    app_ptr load_app(const std::string& id) const;
                    bool save_app(const app&);
                    bool clone_app(const app&);
                    void export_app(const app&, const std::string& file);
                    app_ptr import_app(const std::string& file);

                public:
                    user::user_service_ptr user_service();

                private:
                    void fire_apps_updated_event();

                private:
                    void load_apps();

                private:
                    std::string _app_home;
                    std::string _tmp_app_home;
                    app_metadata_map _app_metadata;

                    user::user_service_ptr _user_service;
                    messages::sender_ptr _sender;

                    mutable util::disk_store _local_data;
                    mutable std::mutex _mutex;
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

