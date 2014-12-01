/*
 * Copyright (C) 2014  Maxim Noah Khailo
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

#include "gui/lua/base.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            namespace 
            {
                const std::string ARRAY_K = "__a";
            }

            void observable_ref::set_name(const std::string& n)
            {
                INVARIANT(api);

                auto obs = api->get_observable(id);
                if(!obs) return;

                _name = n;
                obs->_name = n;
                api->observable_names[n] = id;
            }

            std::string observable_ref::get_name() const
            {
                return _name;
            }

            bool widget_ref::enabled()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->is_widget_enabled(id);
            }

            void widget_ref::enable()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->widget_enable(id, true);
            }

            void widget_ref::disable()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->widget_enable(id, false);
            }

            bool widget_ref::visible()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->is_widget_visible(id);
            }

            void widget_ref::show()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->widget_visible(id, true);
            }

            void widget_ref::hide()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->widget_visible(id, false);
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

            std::string contact_ref::get_id() const
            {
                INVARIANT(api);
                return user_id;
            }

            std::string contact_ref::get_name() const
            {
                INVARIANT(api);
                INVARIANT(api->conversation);

                if(is_self) 
                    return api->conversation->user_service()->user().info().name();

                //otherwise find user
                auto c = api->conversation->contacts().by_id(user_id);
                if(!c) return "";

                return c->name();
            }

            bool contact_ref::is_online() const
            {
                INVARIANT(api);
                INVARIANT(api->conversation);

                if(is_self) return true;

                return api->conversation->user_service()->contact_available(user_id);
            }

            size_t bin_data::get_size() const
            {
                return data.size();
            }

            char bin_data::get(size_t i) const
            {
                return i >= data.size() ? 0 : data[i];
            }

            void bin_data::set(size_t i, char c)
            {
                if(i < data.size()) data[i] = c;
            }

            std::string bin_data::to_str() const
            {
                return std::string{data.begin(), data.end()};
            }

            bin_data bin_data::sub(size_t p, size_t s) const
            {
                auto si = data.begin(); std::advance(si, p);
                auto ei = si;           std::advance(ei, s);
                return {{si, ei}};
            }

            void bin_data::append(const bin_data& n) 
            {
                data.insert(data.end(), n.data.begin(), n.data.end());
            }

            std::string bin_file_data_wrapper::get_name() const
            {
                return file.name;
            }

            size_t bin_file_data_wrapper::get_size() const
            {
                return file.data.size();
            }

            bin_data bin_file_data_wrapper::get_data() const
            {
                return bin_data{file.data};
            }
 
            bool bin_file_data_wrapper::is_good() const
            {
                return file.good;
            }

            std::string file_data_wrapper::get_name() const
            {
                return file.name;
            }

            size_t file_data_wrapper::get_size() const
            {
                return file.data.size();
            }

            std::string file_data_wrapper::get_data() const
            {
                return file.data;
            }
 
            bool file_data_wrapper::is_good() const
            {
                return file.good;
            }

            const std::string EVENT_MESSAGE = "#e";
            event_message::event_message(
                    const std::string& obj,
                    const std::string& type,
                    const u::value& v,
                    lua_api* api) : 
                _obj{obj}, _type{type}, _v{v}, _api{api} 
                { 
                    REQUIRE(_api);
                    INVARIANT(_api);
                }

            event_message::event_message(const m::message& m, lua_api* api) :
                _api{api}
            {
                REQUIRE(api);
                REQUIRE_EQUAL(m.meta.type, EVENT_MESSAGE);

                _obj = m.meta.extra["o"].as_string();
                _type = m.meta.extra["t"].as_string();
                u::decode(m.data, _v);

                INVARIANT(_api);
            }
            
            event_message::operator m::message() const
            {
                m::message m; 
                m.meta.type = EVENT_MESSAGE;
                m.meta.extra["o"] = _obj;
                m.meta.extra["t"] = _type;
                m.data = u::encode(_v);
                return m;
            }

            const std::string SCRIPT_MESSAGE = "#s";
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
                if(m.meta.extra.has("t"))
                    _type = m.meta.extra["t"].as_string();
                if(m.meta.extra.has("local_app_id")) 
                    _local_app_id = m.meta.extra["local_app_id"].as_string();

                u::decode(m.data, _v);

                INVARIANT(_api);
            }
            
            script_message::operator m::message() const
            {
                m::message m; 
                m.meta.type = SCRIPT_MESSAGE;
                if(!_type.empty()) m.meta.extra["t"] = _type;
                m.data = u::encode(_v);
                m.meta.robust = _robust;
                return m;
            }

            void script_message::not_robust() 
            {
                _robust = false;
            }

            const u::value& script_message::get(const std::string& k) const
            {
                return _v[k];
            }

            void script_message::set(const std::string& k, const u::value& v) 
            {
                _v[k] = v;
            }

            bool script_message::has(const std::string& k) const 
            {
                return _v.has(k);
            }

            bin_data script_message::get_bin(const std::string& k) const
            {
                if(!_v.has(k)) return bin_data{};

                auto v = _v[k];
                if(!v.is_bytes()) return bin_data{};
                return bin_data{v.as_bytes()};
            }

            void script_message::set_bin(const std::string& k, const bin_data& v) 
            {
                _v[k] = v.data;
            }

            void script_message::set_vclock(const std::string& k, const vclock_wrapper& c)
            {
                _v[k] = u::to_dict(c.clock());
            }

            vclock_wrapper script_message::get_vclock(const std::string& k) const
            try
            {
                if(!_v.has(k)) return vclock_wrapper{};
                
                auto v = _v[k];
                if(!v.is_dict()) return vclock_wrapper{};
                return vclock_wrapper{u::to_tracked_sclock(v.as_dict())};
            }
            catch(...)
            {
                return vclock_wrapper{};
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
                INVARIANT(_api->conversation);

                auto c = _api->conversation->contacts().by_id(_from_id);
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

            void script_message::set_type(const std::string& t)
            {
                _type = t;
            }

            const std::string& script_message::get_type() const
            {
                return _type;
            }


            store_ref::store_ref(u::disk_store& d) : _d(d){}

            std::string store_ref::get(const std::string& k) const
            {
                if(!_d.has(k)) return "";
                return _d.get(k).as_string();
            }

            void store_ref::set(const std::string& k, const std::string& v) 
            {
                _d.set(k, v);
            }

            bin_data store_ref::get_bin(const std::string& k) const

            {
                if(!_d.has(k)) return bin_data{};

                auto v = _d.get(k);
                if(!v.is_bytes()) return bin_data{};
                return bin_data{v.as_bytes()};
            }

            void store_ref::set_bin(const std::string& k, const bin_data& v) 
            {
                _d.set(k,v.data);
            }

            void store_ref::set_vclock(const std::string& k, const vclock_wrapper& c)
            {
                _d.set(k, u::to_dict(c.clock()));
            }

            vclock_wrapper store_ref::get_vclock(const std::string& k) const
            try
            {
                if(!_d.has(k)) return vclock_wrapper{};
                
                auto v = _d.get(k);
                if(!v.is_dict()) return vclock_wrapper{};
                return vclock_wrapper{u::to_tracked_sclock(v.as_dict())};
            }
            catch(...)
            {
                return vclock_wrapper{};
            }

            bool store_ref::has(const std::string& k) const
            {
                return _d.has(k);
            }

            bool store_ref::remove(const std::string& k)
            {
                return _d.remove(k);
            }

            size_t store_ref::size() const
            {
                return _d.size();
            }

            const util::disk_store& store_ref::store() const
            {
                return _d;
            }

            util::disk_store& store_ref::store()
            {
                return _d;
            }

            u::dict to_dict(lua_State* L, int table)
            {
                REQUIRE(L);

                u::dict d;
                //push first key
                lua_pushnil(L);
                while (lua_next(L, table) != 0)
                {
                    //extract key
                    std::string key;
                    int ik = -1;
                    if(lua_isnumber(L, -2)) ik = lua_tonumber(L, -2); 
                    else key = lua_tostring(L, -2);

                    //extract value
                    u::value v;
                    if(lua_isnumber(L, -1)) v = lua_tonumber(L, -1);
                    else if(lua_isstring(L, -1)) v = std::string{lua_tostring(L, -1)};
                    else if(lua_istable(L, -1)) v = to_dict(L, lua_gettop(L));
                    else 
                    {
                        lua_pop(L, 1);
                        continue;
                    }

                    //insert to dict
                    if(ik != -1)
                    {
                        u::array* a = nullptr;
                        if(!d.has(ARRAY_K))
                        {
                            u::array na;
                            d[ARRAY_K] = na;
                            a = &d[ARRAY_K].as_array();
                        } 
                        else a = &d[ARRAY_K].as_array();

                        CHECK(a);

                        //lua arrays start with 1
                        if(ik != 0) ik --;
                        if(ik >= static_cast<int>(a->size())) a->resize(ik + 1);

                        (*a)[ik] = v;
                    }
                    else d[key] = v;

                    lua_pop(L, 1);
                }
                return d;
            }

            void push_dict(lua_State* L, const u::dict& d);
            void push_array(lua_State* L, const u::array& a);
            void push_value(lua_State* L, const u::value& v)
            {
                REQUIRE(L);

                if(v.is_double())
                    lua_pushnumber(L, v.as_double());
                else if(v.is_bytes())
                    lua_pushstring(L, v.as_string().c_str());
                else if(v.is_dict())
                    push_dict(L, v.as_dict());
                else if(v.is_array())
                    push_array(L, v.as_array());
            }

            void push_array(lua_State* L, const u::array& a, int table)
            {
                REQUIRE(L);

                int i = 1;
                for(const auto& v : a)
                {
                    lua_pushnumber(L, i);
                    push_value(L, v);
                    lua_settable(L, table);
                    i++;
                }
            }

            void push_array(lua_State* L, const u::array& a)
            {
                REQUIRE(L);

                lua_newtable(L);
                auto table = lua_gettop(L);
                push_array(L, a, table);
            }


            void push_dict(lua_State* L, const u::dict& d)
            {
                REQUIRE(L);

                lua_newtable(L);
                auto table = lua_gettop(L);
                for(const auto& p : d)
                {
                    if(p.first == ARRAY_K) push_array(L, p.second, table);
                    else
                    {
                        lua_pushstring(L, p.first.c_str());
                        push_value(L, p.second);
                        lua_settable(L, table);
                    }
                }
            }

            u::value to_value(lua_State* L, int param)
            {
                REQUIRE(L);

                u::value v;
                if(lua_istable(L, param)) v = to_dict(L, param);
                else if(lua_isnumber(L, param)) v = lua_tonumber(L, param);
                else if(lua_isstring(L, param)) v = std::string{lua_tostring(L, param)};
                else if(lua_isboolean(L, param)) v = lua_toboolean(L, param) ? 1 : 0;
                return v;
            }

            int store_ref_set(lua_State* L)
            {
                REQUIRE(L);

                auto params = lua_gettop(L);
                if(params != 3) return 0;

                //get store ref
                store_ref* r = SLB::Private::Type<store_ref*>::get(L, 1);
                if(r == nullptr) return 0;
                if(!lua_isstring(L, 2)) return 0;

                //get key
                std::string key = lua_tostring(L, 2);
                if(key.empty()) return 0;

                //set value
                u::value v = to_value(L, 3);
                r->store().set(key, v);

                return 0;
            }

            int store_ref_get(lua_State* L)
            {
                REQUIRE(L);

                auto params = lua_gettop(L);
                if(params != 2) { lua_pushnil(L); return 1; }

                //get store ref
                store_ref* r = SLB::Private::Type<store_ref*>::get(L, 1);
                if(!r) { lua_pushnil(L); return 1; }
                if(!lua_isstring(L, 2)) { lua_pushnil(L); return 1; }

                //get key
                std::string key = lua_tostring(L, 2);
                if(!r->store().has(key)) { lua_pushnil(L); return 1;} 

                //return value
                auto v = r->store().get(key);
                push_value(L, v);
                return 1;
            }

            int script_message_set(lua_State* L)
            {
                REQUIRE(L);
                auto params = lua_gettop(L);
                if(params != 3) return 0;

                //get script message
                script_message* r = SLB::Private::Type<script_message*>::get(L, 1);
                if(r == nullptr) return 0;
                if(!lua_isstring(L, 2)) return 0;

                //get key
                std::string key = lua_tostring(L, 2);
                if(key.empty()) return 0;

                //set value
                u::value v = to_value(L, 3);
                r->set(key, v);

                return 0;
            }

            int script_message_get(lua_State* L)
            {
                auto params = lua_gettop(L);
                if(params != 2) { lua_pushnil(L); return 1; }

                //get script message
                script_message* r = SLB::Private::Type<script_message*>::get(L, 1);
                if(!r) { lua_pushnil(L); return 1; }
                if(!lua_isstring(L, 2)) { lua_pushnil(L); return 1; }

                //get key
                std::string key = lua_tostring(L, 2);
                if(!r->has(key)) { lua_pushnil(L); return 1;} 

                //return value
                auto v = r->get(key);
                push_value(L, v);
                return 1;
            }

            vclock_wrapper::vclock_wrapper(const u::tracked_sclock& c) : _good{true}, _c{c} { }
            vclock_wrapper::vclock_wrapper(const std::string& id) : _good{true}, _c{id} { }
            vclock_wrapper::vclock_wrapper() : _good{false}, _c{""} { }

            u::tracked_sclock& vclock_wrapper::clock()
            {
                return _c;
            }

            const u::tracked_sclock& vclock_wrapper::clock() const
            {
                return _c;
            }

            bool vclock_wrapper::good() const
            {
                return _good;
            }

            void vclock_wrapper::increment()
            {
                _c++;
            }

            void vclock_wrapper::merge(const vclock_wrapper& o)
            {
                _c += o._c;
            }

            bool vclock_wrapper::conflict(const vclock_wrapper& o)
            {
                return _c.conflict(o._c);
            }

            bool vclock_wrapper::concurrent(const vclock_wrapper& o)
            {
                return _c.concurrent(o._c);
            }

            int vclock_wrapper::compare(const vclock_wrapper& o)
            {
                return _c.compare(o._c);
            }

            bool vclock_wrapper::same(const vclock_wrapper& o)
            {
                return _c.identical(o._c);
            }
        }
    }
}
