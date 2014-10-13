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

#include "gui/app/app_service.hpp"
#include "gui/util.hpp"

#include "util/dbc.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/uuid.hpp"

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
                const std::string LOCAL_DATA = "data";
                const std::string TMP_APP_HOME = "tmp";
                const std::string DATA_DIR = "data";
            }

            std::string get_app_home(bf::path home)
            {
                bf::path app_home = home / APP_HOME;
                return app_home.string();
            }

            std::string get_tmp_app_home(bf::path home)
            {
                bf::path app_home = home / TMP_APP_HOME;
                return app_home.string();
            }

            std::string get_local_data_dir(bf::path home)
            {
                bf::path app_home = home / LOCAL_DATA;
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
                _tmp_app_home = get_tmp_app_home(_user_service->home());

                //setup local data store
                auto local_data_dir = get_local_data_dir(_user_service->home());
                u::create_directory(local_data_dir);
                _local_data.load(local_data_dir);

                //load app metadata
                u::create_directory(_app_home);
                load_apps();

                init_handlers();
                start();

                INVARIANT(_user_service);
                INVARIANT(_sender);
                INVARIANT_FALSE(_app_home.empty());
                INVARIANT_FALSE(_tmp_app_home.empty());
            }

            void app_service::init_handlers()
            {
                using std::bind;
                using namespace std::placeholders;

                //sevices require at least one handler
                //so creating a nop to satify requirements
                handle("_", bind(&app_service::received_nop, this, _1));
            }

            void app_service::received_nop(const m::message& m)
            {
                //do nothing ATM
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

            std::string get_app_dir(bf::path root, const std::string& id)
            {
                bf::path d = root / id;
                return d.string();
            }

            std::string get_app_data_dir(bf::path root)
            {
                bf::path d = root / DATA_DIR;
                return d.string();
            }

            std::string get_app_dir(bf::path root, const app& a)
            {
                return get_app_dir(root, a.id());
            }

            app_ptr app_service::load_app(const std::string& id) const
            {
                u::mutex_scoped_lock l(_mutex);

                auto p = _app_metadata.find(id);
                if(p == _app_metadata.end()) return nullptr;

                LOG << "loading app `" << p->second.name << "' (" << p->second.id << ") from `" << p->second.path << "'" << std::endl;

                return a::load_app(_local_data, p->second.path);
            }

            std::string setup_tmp_dir(
                    const m::message& m, 
                    const std::string& tmp_root)
            {
                auto id = m.meta.extra["app_id"].as_string();
                auto t = get_app_dir(tmp_root, id);
                auto d = get_app_data_dir(t);
                u::create_directory(d);
                return t;
            }

            void setup_tmp_app(app& a, const std::string& tmp_root)
            {
                auto t = get_app_dir(tmp_root, a);
                auto d = get_app_data_dir(t);
                u::create_directory(d);
                a.path(t);
                a.data().load(d);
                a.set_tmp();
            }

            //create app from message
            app_ptr app_service::create_app(const m::message& m) const
            {
                u::mutex_scoped_lock l(_mutex);

                auto tmp_dir = setup_tmp_dir(m, _tmp_app_home);
                auto a = std::make_shared<app>(_local_data, tmp_dir, m);
                a->set_tmp();
                ENSURE(a);
                return a;
            }

            app_ptr app_service::create_new_app() const
            {
                u::mutex_scoped_lock l(_mutex);
                auto a = std::make_shared<app>(_local_data);

                //setup tmp app directory
                setup_tmp_app(*a, _tmp_app_home);
                ENSURE(a);
                return a;
            }

            bool app_service::save_app(const app& a)
            {
                u::mutex_scoped_lock l(_mutex);

                INVARIANT_FALSE(_app_home.empty());
                
                auto app_dir = get_app_dir(_app_home, a);
                LOG << "saving app `" << a.name() << "' (" << a.id() << ") to `" << app_dir << "'" << std::endl;
                u::create_directory(app_dir);
                bool saved = a::save_app(app_dir, a);

                app_metadata m;
                if(!load_app_metadata(app_dir, m)) return false;

                _app_metadata[m.id] = m;
                fire_apps_updated_event();
                return saved;
            }

            bool app_service::clone_app(const app& a)
            {
                INVARIANT_FALSE(_app_home.empty());
                REQUIRE_FALSE(a.name().empty());

                return save_app(a.clone());
            }

            void app_service::export_app(const app& a, const std::string& file)
            {
                export_app_as_message(file, a);
            }

            app_ptr app_service::import_app(const std::string& file)
            {
                auto m = import_app_as_message(file);
                auto tmp_dir = setup_tmp_dir(m, _tmp_app_home);
                auto a = std::make_shared<app>(_local_data, tmp_dir, m);
                a->set_tmp();
                return a;
            }

            void app_service::fire_apps_updated_event()
            {
                event::apps_updated e;
                send_event(e.to_message());
            }

            namespace event
            {
                const std::string APPS_UPDATED = "apps_updated";
            }

        }
    }
}

