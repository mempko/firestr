/*
 * Copyright (C) 2013  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vedit_refsion 3 of the License, or
 * (at your option) any later vedit_refsion.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "gui/lua/base.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            void set_enabled(int id, widget_map& map, bool enabled)
            {
                auto w = get_widget<QWidget>(id, map);
                if(!w) return;

                w->setEnabled(enabled);
            }

            bool widget_ref::enabled()
            {
                INVARIANT(api);
                auto w = get_widget<QWidget>(id, api->widgets);
                return w ? w->isEnabled() : false;
            }

            void widget_ref::enable()
            {
                INVARIANT(api);
                set_enabled(id, api->widgets, true);
            }

            void widget_ref::disable()
            {
                INVARIANT(api);
                set_enabled(id, api->widgets, false);
            }

            std::string app_ref::get_id() const
            {
                return app_id;
            }

            void app_ref::send(const script_message& m)
            {
                INVARIANT(api);
                INVARIANT(api->sender);
                api->sender->send_to_local_app(app_id, m); 
            }

            std::string contact_ref::get_name() const
            {
                INVARIANT(api);

                auto c = api->contacts.by_id(user_id);
                if(!c) return "";

                return c->name();
            }

            bool contact_ref::is_online() const
            {
                INVARIANT(api);
                INVARIANT(api->session);

                return api->session->user_service()->contact_available(user_id);
            }

            const std::string SCRIPT_MESSAGE = "script_msg";
            script_message::script_message(lua_api* api) : 
                _from_id{}, _v{}, _api{api} 
                { 
                    REQUIRE(_api);
                    INVARIANT(_api);
                }

            script_message::script_message(const m::message& m, lua_api* api) :
                _api{api}
            {
                REQUIRE(api);
                REQUIRE_EQUAL(m.meta.type, SCRIPT_MESSAGE);

                _from_id = m.meta.extra["from_id"].as_string();
                if(m.meta.extra.has("local_app_id")) 
                    _local_app_id = m.meta.extra["local_app_id"].as_string();

                u::decode(m.data, _v);

                INVARIANT(_api);
            }
            
            script_message::operator m::message() const
            {
                m::message m; 
                m.meta.type = SCRIPT_MESSAGE;
                m.data = u::encode(_v);
                return m;
            }

            std::string script_message::get(const std::string& k) const
            {
                if(!_v.has(k)) return "";
                return _v[k].as_string();
            }

            void script_message::set(const std::string& k, const std::string& v) 
            {
                _v[k] = v;
            }

            contact_ref empty_contact_ref(lua_api& api)
            {
                contact_ref e;
                e.api = &api;
                e.id = 0;
                e.user_id = "0";
                return e;
            }

            contact_ref script_message::from() const
            {
                INVARIANT(_api);

                auto c = _api->contacts.by_id(_from_id);
                if(!c || is_local()) return empty_contact_ref(*_api);

                contact_ref r;
                r.id = 0;
                r.user_id = c->id();
                r.api = _api;

                ENSURE_EQUAL(r.api, _api);
                ENSURE_FALSE(r.user_id.empty());
                return r;
            }

            bool script_message::is_local() const
            {
                return !_local_app_id.empty();
            }

            app_ref empty_app_ref(lua_api& api)
            {
                app_ref e;
                e.api = &api;
                e.id = 0;
                e.app_id = "0";
                return e;
            }

            app_ref script_message::app() const
            {
                INVARIANT(_api);

                if(!is_local()) return empty_app_ref(*_api);

                app_ref r;
                r.id = 0;
                r.app_id = _local_app_id;
                r.api = _api;

                ENSURE_EQUAL(r.api, _api);
                ENSURE_FALSE(r.app_id.empty());
                return r;
            }
        }
    }
}
