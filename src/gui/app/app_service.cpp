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

#include "gui/app/app_service.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

#include <stdexcept>
#include <boost/filesystem.hpp>

namespace s = fire::service;
namespace u = fire::util;
namespace us = fire::user;
namespace m = fire::message;
namespace ms = fire::messages;
namespace a = fire::gui::app;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            namespace 
            {
                const std::string SERVICE_ADDRESS = "app_service";
                const std::string APP_HOME = "apps";
            }

            bool create_directory(const std::string& dir)
            {
                REQUIRE_FALSE(dir.empty());

                if(bf::exists(dir)) return true;

                bf::create_directories(dir);

                return bf::exists(dir);
            }

            std::string get_app_home(bf::path home)
            {
                bf::path app_home = home / APP_HOME;
                return app_home.string();
            }

            app_service::app_service(
                    user::user_service_ptr user_service,
                    message::mailbox_ptr event) :
                s::service{SERVICE_ADDRESS, event},
                _user_service{user_service}
            {
                REQUIRE(user_service);
                REQUIRE(mail());

                _sender = std::make_shared<ms::sender>(_user_service, mail());
                _app_home = get_app_home(_user_service->home());
                create_directory(_app_home);
                load_apps();

                INVARIANT(_user_service);
                INVARIANT(_sender);
                INVARIANT_FALSE(_app_home.empty());
            }

            void app_service::message_recieved(const message::message& m)
            {
                INVARIANT(mail());
                INVARIANT(_user_service);
            }

            user::user_service_ptr app_service::user_service()
            {
                ENSURE(_user_service);
                return _user_service;
            }

            app_metadata_map app_service::available_apps() const
            {
                u::mutex_scoped_lock l(_mutex);
                return _app_metadata;
            }

            void app_service::load_apps()
            {
                bf::path app_dir = _app_home;
                bf::directory_iterator end;
                for (bf::directory_iterator d(app_dir); d != end; ++d)
                {
                    if (!bf::is_directory(*d)) continue;

                    app_metadata m;
                    if(!load_app_metadata(d->path().string(), m)) continue;

                    _app_metadata[m.id] = m;
                } 
            }

            std::string get_app_dir(bf::path home, const std::string& id)
            {
                bf::path d = home / id;
                return d.string();
            }

            app_ptr app_service::load_app(const std::string& id) const
            {
                u::mutex_scoped_lock l(_mutex);

                auto p = _app_metadata.find(id);
                if(p == _app_metadata.end()) return nullptr;

                auto app_dir = get_app_dir(_app_home, p->second.id);
                return a::load_app(app_dir);
            }

            bool app_service::save_app(const app& a)
            {
                u::mutex_scoped_lock l(_mutex);

                INVARIANT_FALSE(_app_home.empty());
                
                auto app_dir = get_app_dir(_app_home, a.id());
                create_directory(app_dir);
                bool saved = a::save_app(app_dir, a);

                app_metadata m;
                if(!load_app_metadata(app_dir, m)) return false;

                _app_metadata[m.id] = m;
                fire_apps_updated_event();
                return saved;
            }

            bool app_service::clone_app(app& a)
            {
                INVARIANT_FALSE(_app_home.empty());
                REQUIRE_FALSE(a.name().empty());

                app na; //make new id
                na.name(a.name());
                na.code(a.code());
                a = na;
                return save_app(a);
            }

            void app_service::fire_apps_updated_event()
            {
                event::apps_updated e;
                send_event(event::convert(e));
            }

            namespace event
            {
                const std::string APPS_UPDATED = "apps_updated";

                m::message convert(const apps_updated&)
                {
                    m::message m;
                    m.meta.type = APPS_UPDATED;
                    return m;
                }

                void convert(const m::message& m, apps_updated& e)
                {
                    REQUIRE_EQUAL(m.meta.type, APPS_UPDATED);
                }
            }
        }
    }
}

