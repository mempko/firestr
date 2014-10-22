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

#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/env.hpp"
#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <functional>
#include <cstdlib>
#include <fstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace a = fire::gui::app;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            lua_api::lua_api(
                    a::app_ptr a,
                    ms::sender_ptr sndr,
                    s::conversation_ptr s,
                    s::conversation_service_ptr ss,
                    api::frontend* f) :
                app{a},
                front{f},
                conversation{s},
                conversation_service{ss},
                sender{sndr},
                _error{}
            {
                REQUIRE(f);
				_error.line = -1;

                INVARIANT(app);
                INVARIANT(sender);
                INVARIANT(conversation);
                INVARIANT(conversation_service);
                INVARIANT(front);

                local_data.reset(new store_ref{app->local_data()});
                data.reset(new store_ref{app->data()});

                bind();

                INVARIANT(state);
                INVARIANT(local_data);
                INVARIANT(data);
            }

            lua_api::~lua_api()
            {
                reset_widgets();
            }

            void lua_api::report_error(const std::string& e, int line)
            {
                _error.line = line;
                _error.message = e;

                LOG << "script error: " << e << std::endl;
                front->report_error(e);
            }

            error_info lua_api::get_error() const
            {
                return _error;
            }

            api::ref_id lua_api::new_id()
            {
                ids++;
                return ids;
            }

            void lua_api::bind()
            {  
                REQUIRE_FALSE(state);
                REQUIRE(data);
                REQUIRE(local_data);

                using namespace std::placeholders;

                SLB::Class<lua_api, SLB::Instance::NoCopyNoDestroy>{"Api", &manager}
                    .set("print", &lua_api::print)
                    .set("alert", &lua_api::alert)
                    .set("button", &lua_api::make_button)
                    .set("label", &lua_api::make_label)
                    .set("edit", &lua_api::make_edit)
                    .set("text_edit", &lua_api::make_text_edit)
                    .set("list", &lua_api::make_list)
                    .set("grid", &lua_api::make_grid)
                    .set("draw", &lua_api::make_draw)
                    .set("pen", &lua_api::make_pen)
                    .set("timer", &lua_api::make_timer)
                    .set("image", &lua_api::make_image)
                    .set("mic", &lua_api::make_mic)
                    .set("speaker", &lua_api::make_speaker)
                    .set("audio_encoder", &lua_api::make_audio_encoder)
                    .set("audio_decoder", &lua_api::make_audio_decoder)
                    .set("vclock", &lua_api::make_vclock)
                    .set("place", &lua_api::place)
                    .set("place_across", &lua_api::place_across)
                    .set("height", &lua_api::height)
                    .set("grow", &lua_api::grow)
                    .set("total_contacts", &lua_api::total_contacts)
                    .set("last_contact", &lua_api::last_contact)
                    .set("contact", &lua_api::get_contact)
                    .set("who_started", &lua_api::who_started)
                    .set("self", &lua_api::self)
                    .set("total_apps", &lua_api::total_apps)
                    .set("app", &lua_api::get_app)
                    .set("message", &lua_api::make_message)
                    .set("when_message_received", &lua_api::set_message_callback)
                    .set("when_local_message_received", &lua_api::set_local_message_callback)
                    .set("when_message", &lua_api::set_message_callback_by_type)
                    .set("when_local_message", &lua_api::set_local_message_callback_by_type)
                    .set("send", &lua_api::send_all)
                    .set("send_to", &lua_api::send_to)
                    .set("send_local", &lua_api::send_local)
                    .set("save_file", &lua_api::save_file)
                    .set("open_file", &lua_api::open_file)
                    .set("save_bin_file", &lua_api::save_bin_file)
                    .set("open_bin_file", &lua_api::open_bin_file)
                    .set("i_started", &lua_api::launched_local);

                SLB::Class<pen_ref>{"pen", &manager}
                    .set("set_width", &pen_ref::set_width);

                SLB::Class<contact_ref>{"contact", &manager}
                    .set("id", &contact_ref::get_id)
                    .set("name", &contact_ref::get_name)
                    .set("online", &contact_ref::is_online);

                SLB::Class<app_ref>{"app", &manager}
                    .set("id", &app_ref::get_id)
                    .set("send", &app_ref::send);

                SLB::Class<bin_data>{"bin_data", &manager}
                    .set("size", &bin_data::get_size)
                    .set("get", &bin_data::get)
                    .set("set", &bin_data::set)
                    .set("sub", &bin_data::sub)
                    .set("append", &bin_data::append)
                    .set("str", &bin_data::to_str);

                SLB::Class<script_message>{"script_message", &manager}
                    .set("from", &script_message::from)
                    .set("not_robust", &script_message::not_robust)
                    .set("get_bin", &script_message::get_bin)
                    .set("set_bin", &script_message::set_bin)
                    .set("get_vclock", &script_message::get_vclock)
                    .set("set_vclock", &script_message::set_vclock)
                    .set("is_local", &script_message::is_local)
                    .set("set_type", &script_message::set_type)
                    .set("type", &script_message::get_type)
                    .set("app", &script_message::app)
                    .set("has", &script_message::has)
                    .set("set", script_message_set)
                    .set("get", script_message_get);

                SLB::Class<store_ref>{"store_ref", &manager}
                    .set("get_bin", &store_ref::get_bin)
                    .set("set_bin", &store_ref::set_bin)
                    .set("get_vclock", &store_ref::get_vclock)
                    .set("set_vclock", &store_ref::set_vclock)
                    .set("has", &store_ref::has)
                    .set("remove", &store_ref::remove)
                    .set("set", store_ref_set)
                    .set("get", store_ref_get);

                SLB::Class<grid_ref>{"grid", &manager}
                    .set("place", &grid_ref::place)
                    .set("place_across", &grid_ref::place_across)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<button_ref>{"button", &manager}
                    .set("text", &button_ref::get_text)
                    .set("set_text", &button_ref::set_text)
                    .set("set_image", &button_ref::set_image)
                    .set("callback", &button_ref::get_callback)
                    .set("when_clicked", &button_ref::set_callback)
                    .set("set_name", &observable_ref::set_name)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<label_ref>{"label", &manager}
                    .set("text", &label_ref::get_text)
                    .set("set_text", &label_ref::set_text)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<edit_ref>{"edit", &manager}
                    .set("text", &edit_ref::get_text)
                    .set("set_text", &edit_ref::set_text)
                    .set("edited_callback", &edit_ref::get_edited_callback)
                    .set("when_edited", &edit_ref::set_edited_callback)
                    .set("finished_callback", &edit_ref::get_finished_callback)
                    .set("when_finished", &edit_ref::set_finished_callback)
                    .set("set_name", &observable_ref::set_name)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<text_edit_ref>{"text_edit", &manager}
                    .set("text", &text_edit_ref::get_text)
                    .set("set_text", &text_edit_ref::set_text)
                    .set("edited_callback", &text_edit_ref::get_edited_callback)
                    .set("when_edited", &text_edit_ref::set_edited_callback)
                    .set("set_name", &observable_ref::set_name)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<list_ref>{"list_ref", &manager}
                    .set("add", &list_ref::add)
                    .set("remove", &list_ref::remove)
                    .set("size", &list_ref::size)
                    .set("clear", &list_ref::clear)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<draw_ref>{"draw", &manager}
                    .set("mouse_moved_callback", &draw_ref::get_mouse_moved_callback)
                    .set("mouse_pressed_callback", &draw_ref::get_mouse_pressed_callback)
                    .set("mouse_released_callback", &draw_ref::get_mouse_released_callback)
                    .set("mouse_dragged_callback", &draw_ref::get_mouse_dragged_callback)
                    .set("when_mouse_moved", &draw_ref::set_mouse_moved_callback)
                    .set("when_mouse_pressed", &draw_ref::set_mouse_pressed_callback)
                    .set("when_mouse_released", &draw_ref::set_mouse_released_callback)
                    .set("when_mouse_dragged", &draw_ref::set_mouse_dragged_callback)
                    .set("set_name", &observable_ref::set_name)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable)
                    .set("clear", &draw_ref::clear)
                    .set("line", &draw_ref::line)
                    .set("circle", &draw_ref::circle)
                    .set("image", &draw_ref::image)
                    .set("pen", &draw_ref::set_pen)
                    .set("get_pen", &draw_ref::get_pen);

                SLB::Class<image_ref>{"image_ref", &manager}
                    .set("enabled", &image_ref::enabled)
                    .set("enable", &image_ref::enable)
                    .set("disable", &image_ref::disable)
                    .set("width", &image_ref::width)
                    .set("height", &image_ref::height)
                    .set("good", &image_ref::good);

                SLB::Class<timer_ref>{"timer_ref", &manager}
                    .set("running", &timer_ref::running)
                    .set("start", &timer_ref::start)
                    .set("stop", &timer_ref::stop)
                    .set("interval", &timer_ref::set_interval)
                    .set("when_triggered", &timer_ref::set_callback);

                SLB::Class<bin_file_data_wrapper>{"bin_file_data", &manager}
                    .set("good", &bin_file_data_wrapper::is_good)
                    .set("name", &bin_file_data_wrapper::get_name)
                    .set("size", &bin_file_data_wrapper::get_size)
                    .set("data", &bin_file_data_wrapper::get_data);

                SLB::Class<file_data_wrapper>{"file_data", &manager}
                    .set("good", &file_data_wrapper::is_good)
                    .set("name", &file_data_wrapper::get_name)
                    .set("size", &file_data_wrapper::get_size)
                    .set("data", &file_data_wrapper::get_data);

                SLB::Class<microphone_ref>{"microphone_ref", &manager}
                    .set("when_sound", &microphone_ref::set_callback)
                    .set("start", &microphone_ref::start)
                    .set("stop", &microphone_ref::stop);

                SLB::Class<speaker_ref>{"speaker_ref", &manager}
                    .set("play", &speaker_ref::play)
                    .set("mute", &speaker_ref::mute)
                    .set("unmute", &speaker_ref::unmute);

                SLB::Class<opus_encoder_wrapper>{"audio_encoder", &manager}
                    .set("encode", &opus_encoder_wrapper::encode);

                SLB::Class<opus_decoder_wrapper>{"audio_decoder", &manager}
                    .set("decode", &opus_decoder_wrapper::decode);

                SLB::Class<vclock_wrapper>{"vclock", &manager}
                    .set("good", &vclock_wrapper::good)
                    .set("inc", &vclock_wrapper::increment)
                    .set("merge", &vclock_wrapper::merge)
                    .set("conflict", &vclock_wrapper::conflict)
                    .set("concurrent", &vclock_wrapper::concurrent)
                    .set("comp", &vclock_wrapper::compare)
                    .set("equals", &vclock_wrapper::same);

                state = std::make_shared<SLB::Script>(&manager);
                state->set("app", this);
                state->set("store", local_data.get());
                state->set("data", data.get());
                ENSURE(state);
            }

            error_info lua_api::execute(const std::string& s)
            try
            {
                REQUIRE_FALSE(s.empty());
                INVARIANT(state);

                return 
                    state->safeDoString(s.c_str(), "app") ? 
                    error_info{-1, ""} : error_info{state->getLastErrorLine(), state->getLastError()};
            }
            catch(std::exception& e)
            {
                return {-1, e.what()};
            }
            catch(...)
            {
                return {-1, "unknown"};
            }

            void lua_api::reset_widgets()
            {
                INVARIANT(front);
                front->reset();

                button_refs.clear();
                edit_refs.clear();
                text_edit_refs.clear();
                list_refs.clear();
                timer_refs.clear();
                grid_refs.clear();
                image_refs.clear();
                observable_names.clear();
                

                ENSURE(edit_refs.empty());
                ENSURE(text_edit_refs.empty());
                ENSURE(list_refs.empty());
                ENSURE(timer_refs.empty());
                ENSURE(grid_refs.empty());
                ENSURE(image_refs.empty());
            }

            void lua_api::run(const std::string& code)
            {
                REQUIRE_FALSE(code.empty());
                INVARIANT(front);
                auto error = execute(code);

                if(error.message.empty())
                {
                    _error.line = -1;
                    _error.message.clear();
                    return;
                }
                report_error(error.message, error.line);

            }

            //API implementation 
            void lua_api::print(const std::string& a)
            {
                INVARIANT(front);

                LOG << "script: " << a << std::endl;
                front->print(a);
            }

            void lua_api::alert()
            {
                INVARIANT(conversation);
                INVARIANT(conversation_service);

                conversation_service->fire_conversation_alert(conversation->id(), visible());
            }

            bool lua_api::visible() const
            {
                INVARIANT(front);
                return front->visible();
            }

            void lua_api::message_received(const script_message& m)
            try
            {
                INVARIANT(state);
                std::string callback;
                //if a message type is set, try to find a callback in the callback map
                //for that type
                if(!m.get_type().empty())
                {
                    if(m.is_local())
                    {
                        auto ci = local_message_callbacks.find(m.get_type());
                        if(ci != local_message_callbacks.end()) callback = ci->second;
                    }
                    else
                    {
                        auto ci = message_callbacks.find(m.get_type());
                        if(ci != message_callbacks.end()) callback = ci->second;
                    }
                }

                //if no callback matched the type, use the generic callback
                if(callback.empty())
                {
                    if(m.is_local()) callback = local_message_callback;
                    else callback = message_callback;
                }

                //if there is no callback set, message is ignored.
                if(callback.empty()) return;
                state->call(callback, m);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in message_received: " << e.what();
                report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in message_received: unknown", state->getLastErrorLine());
            }

            void lua_api::event_received(const event_message& e)
            try
            {
                auto i = observable_names.find(e.obj());
                if(i == observable_names.end()) return;

                auto obs = get_observable(i->second);
                if(!obs) return;

                obs->handle(e.type(), e.value());
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in event_received: " << e.what();
                report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in event recieved: unknown", state->getLastErrorLine());
            }

            void lua_api::set_message_callback(const std::string& a)
            {
                message_callback = a;
            }

            void lua_api::set_message_callback_by_type(const std::string& t,  const std::string& a)
            {
                message_callbacks[t] = a;
            }

            void lua_api::set_local_message_callback(const std::string& a)
            {
                local_message_callback = a;
            }

            void lua_api::set_local_message_callback_by_type(const std::string& t,  const std::string& a)
            {
                local_message_callbacks[t] = a;
            }

            script_message lua_api::make_message()
            {
                return {this};
            }

            void lua_api::send_to_helper(us::user_info_ptr c, const script_message& m)
            {
                REQUIRE(c);
                INVARIANT(conversation);
                if( !conversation->user_service()->contact_available(c->id()) || 
                    !conversation->contacts().has(c->id()))
                    return;

                sender->send(c->id(), m); 
            }

            void lua_api::send_all(const script_message& m)
            {
                INVARIANT(sender);
                INVARIANT(conversation);
                for(auto c : conversation->contacts().list())
                {
                    CHECK(c);
                    send_to_helper(c, m);
                }
            }

            void lua_api::send(const event_message& m)
            {
                INVARIANT(sender);
                INVARIANT(conversation);
                for(auto c : conversation->contacts().list())
                {
                    CHECK(c);
                    if(!conversation->user_service()->contact_available(c->id()) || 
                       !conversation->contacts().has(c->id()))
                        continue;
                    sender->send(c->id(), m);
                }
            }

            void lua_api::send_simple_event(const std::string& name, const std::string& type)
            {
                if(name.empty()) return;
                u::value v;
                event_message em{name, type, v, this};
                send(em);
            }

            void lua_api::send_to(const contact_ref& cr, const script_message& m)
            {
                INVARIANT(sender);
                INVARIANT(conversation);

                auto c = conversation->contacts().by_id(cr.user_id);
                if(!c) return;
                send_to_helper(c, m);
            }

            size_t lua_api::total_contacts() const
            {
                INVARIANT(conversation);
                return conversation->contacts().size();
            }

            int lua_api::last_contact() const
            {
                INVARIANT(conversation);
                return conversation->contacts().size() - 1;
            }

            contact_ref lua_api::get_contact(size_t i)
            {
                INVARIANT(conversation);

                auto c = conversation->contacts().get(i);
                if(!c) return empty_contact_ref(*this);

                contact_ref r;
                r.id = 0;
                r.user_id = c->id();
                r.api = this;

                ENSURE_EQUAL(r.api, this);
                ENSURE_FALSE(r.user_id.empty());
                return r;
            }

            contact_ref lua_api::self() 
            {
                INVARIANT(app);
                INVARIANT(conversation);
                INVARIANT(conversation->user_service());

                contact_ref r;
                r.id = 0;
                r.user_id = conversation->user_service()->user().info().id();
                r.api = this;
                r.is_self = true;
                return r;
            }

            contact_ref lua_api::who_started() 
            {
                INVARIANT(app);
                INVARIANT(conversation);
                INVARIANT(conversation->user_service());

                contact_ref r;
                if(app->launched_local())
                {
                    r.id = 0;
                    r.user_id = conversation->user_service()->user().info().id();
                    r.api = this;
                    r.is_self = true;
                } 
                else
                {
                    auto c = conversation->contacts().by_id(who_started_id);
                    CHECK(c);
                    r.id = 0;
                    r.user_id = c->id();
                    r.api = this;
                }
                ENSURE_EQUAL(r.api, this);
                ENSURE_FALSE(r.user_id.empty());
                return r;
            }

            size_t lua_api::total_apps() const
            {
                INVARIANT(conversation);

                return conversation->apps().size();
            }

            app_ref lua_api::get_app(size_t i)
            {
                INVARIANT(conversation);
                const auto& apps = conversation->apps();
                if(i >= apps.size()) return empty_app_ref(*this);

                CHECK_FALSE(apps[i].address.empty());

                auto id = apps[i].address;

                app_ref r;
                r.id = 0;
                r.app_id = id;
                r.api = this;

                ENSURE_EQUAL(r.api, this);
                ENSURE_FALSE(r.app_id.empty());
                return r;
            }

            bool lua_api::launched_local() const
            {
                INVARIANT(app);
                return app->launched_local();
            }

            void lua_api::send_local(const script_message& m)
            {
                INVARIANT(conversation);
                for(const auto& app : conversation->apps())
                {
                    CHECK_FALSE(app.address.empty());
                    sender->send_to_local_app(app.address, m);
                }
            }

            grid_ref lua_api::make_grid()
            {
                INVARIANT(front);
                //create button reference
                grid_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                grid_refs[ref.id] = ref;

                front->add_grid(ref.id);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::height(int h)
            {
                INVARIANT(front);
                front->height(h);
            }

            void lua_api::grow()
            {
                INVARIANT(front);
                front->grow();
            }

            void lua_api::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(front);

                front->place(wr.id, r, c);
            }

            void lua_api::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(front);

                front->place_across(wr.id, r, c, row_span, col_span);
            }

            button_ref lua_api::make_button(const std::string& text)
            {
                INVARIANT(front);

                //create button reference
                button_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                button_refs[ref.id] = ref;

                front->add_button(ref.id, text);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            template <class mp>
                observable_ref* get_obs(mp& m, int id)
                {
                    auto b = m.find(id);
                    if(b != m.end()) return &b->second;
                    return nullptr;
                }

            observable_ref* lua_api::get_observable(int id)
            {
                //linear search though sets
                if(auto o = get_obs(button_refs, id)) return o;
                else if(auto o = get_obs(label_refs, id)) return o;
                else if(auto o = get_obs(edit_refs, id)) return o;
                else if(auto o = get_obs(text_edit_refs, id)) return o;
                else if(auto o = get_obs(list_refs, id)) return o;
                else if(auto o = get_obs(grid_refs, id)) return o;
                else if(auto o = get_obs(draw_refs, id)) return o;

                return nullptr;
            }

            label_ref lua_api::make_label(const std::string& text)
            {
                INVARIANT(front);

                //create edit reference
                label_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                label_refs[ref.id] = ref;

                front->add_label(ref.id, text);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            edit_ref lua_api::make_edit(const std::string& text)
            {
                INVARIANT(front);

                //create edit reference
                edit_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                edit_refs[ref.id] = ref;

                front->add_edit(ref.id, text);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.finished_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::edit_edited(api::ref_id id)
            try
            {
                INVARIANT(state);

                auto rp = edit_refs.find(id);
                if(rp == edit_refs.end()) return;

                auto callback = rp->second.edited_callback;
                if(callback.empty()) return;

                auto text = rp->second.get_text();
                auto name = rp->second.get_name();
                if(!name.empty())
                {
                    u::value v = text;
                    event_message em{name, "e", v, this};
                    send(em);
                }

                state->call(callback, text);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in edit_edited: " << e.what();
                report_error(e.what(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in edit_edited: unknown");
            }

            void lua_api::edit_finished(api::ref_id id)
            try
            {
                INVARIANT(state);

                auto rp = edit_refs.find(id);
                if(rp == edit_refs.end()) return;

                auto callback = rp->second.finished_callback;
                if(callback.empty()) return;

                auto text = rp->second.get_text();
                auto name = rp->second.get_name();
                if(!name.empty())
                {
                    u::value v = text;
                    event_message em{name, "f", v, this};
                    send(em);
                }

                state->call(callback, text);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in edit_finished: " << e.what();
                report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in edit_finished: unknown");
            }

            text_edit_ref lua_api::make_text_edit(const std::string& text)
            {
                INVARIANT(front);

                //create edit reference
                text_edit_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                text_edit_refs[ref.id] = ref;

                front->add_text_edit(ref.id, text);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::text_edit_edited(api::ref_id id)
            try
            {
                INVARIANT(state);

                auto rp = text_edit_refs.find(id);
                if(rp == text_edit_refs.end()) return;

                auto callback = rp->second.edited_callback;
                if(callback.empty()) return;

                auto text = rp->second.get_text();
                auto name = rp->second.get_name();
                if(!name.empty())
                {
                    u::value v = text;
                    event_message em{name, "e", v, this};
                    send(em);
                }

                state->call(callback, text);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in text_edit_edited: " << e.what();
                report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in text_edit_edited: unknown");
            }

            list_ref lua_api::make_list()
            {
                INVARIANT(front);

                //create edit reference
                list_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                list_refs[ref.id] = ref;

                front->add_list(ref.id);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            pen_ref lua_api::make_pen(const std::string& color, int width)
            {
                INVARIANT(front);

                pen_ref ref;
                ref.id = new_id();
                ref.api = this;

                front->add_pen(ref.id, color, width);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            draw_ref lua_api::make_draw(int width, int height)
            {
                INVARIANT(front);

                //create edit reference
                draw_ref ref;
                ref.id = new_id();
                ref.api = this;

                //add ref and widget to maps
                draw_refs[ref.id] = ref;

                front->add_draw(ref.id, width, height);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            timer_ref lua_api::make_timer(int msec, const std::string& callback)
            {
                INVARIANT(front);

                //create edit reference
                timer_ref ref;
                ref.id = new_id();
                ref.api = this;
                ref.callback = callback;

                //add ref and widget to maps
                timer_refs[ref.id] = ref;

                front->add_timer(ref.id, msec);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::timer_triggered(api::ref_id id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    auto t = timer_refs.find(id);
                    if(t == timer_refs.end()) return;
                    callback = t->second.callback;
                }

                if(callback.empty()) return;
                run(callback);
            }

            image_ref lua_api::make_image(const bin_data& d)
            {
                INVARIANT(front);
                
                //create edit reference
                image_ref ref;
                ref.id = new_id();
                ref.api = this;
                ref.w = 0;
                ref.h = 0;

                image_refs[ref.id] = ref;

                ref.g = front->add_image(ref.id, d.data);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            microphone_ref lua_api::make_mic(const std::string& callback, const std::string& codec)
            {
                INVARIANT(front);

                microphone_ref ref;
                ref.id = new_id();
                ref.api = this;
                ref.callback = callback;
                mic_refs[ref.id] = ref;

                front->add_mic(ref.id, codec);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            opus_encoder_wrapper lua_api::make_audio_encoder()
            {
                return opus_encoder_wrapper{};
            }

            opus_decoder_wrapper lua_api::make_audio_decoder()
            {
                return opus_decoder_wrapper{};
            }

            vclock_wrapper lua_api::make_vclock()
            {
                INVARIANT(conversation);
                INVARIANT(conversation->user_service());
                return vclock_wrapper{conversation->user_service()->user().info().id()};
            }

            speaker_ref lua_api::make_speaker(const std::string& codec)
            {
                INVARIANT(front);

                speaker_ref ref;
                ref.id = new_id();
                ref.api = this;
                speaker_refs[ref.id] = ref;

                front->add_speaker(ref.id, codec);

                return ref;
            }

            void lua_api::got_sound(api::ref_id id, const u::bytes& bd)
            try
            {
                auto mp = mic_refs.find(id);
                if(mp == mic_refs.end()) return;

                const auto& ref = mp->second;
                if(ref.callback.empty()) return;

                state->call(ref.callback, bin_data{bd});
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in got_sound: " << e.what();
                report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                report_error("error in got_sound: unknown", state->getLastErrorLine());
            }

            void lua_api::draw_mouse_pressed(api::ref_id id, int button, int x, int y)
            {
                auto dp = draw_refs.find(id);
                if(dp == draw_refs.end()) return;

                auto name = dp->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["b"] = button;
                    d["x"] = x;
                    d["y"] = y;
                    event_message em{name, "p", d, this};
                    send(em);
                }

                dp->second.mouse_pressed(button, x, y);
            }

            void lua_api::draw_mouse_released(api::ref_id id, int button, int x, int y)
            {
                auto dp = draw_refs.find(id);
                if(dp == draw_refs.end()) return;

                auto name = dp->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["b"] = button;
                    d["x"] = x;
                    d["y"] = y;
                    event_message em{name, "r", d, this};
                    send(em);
                }

                dp->second.mouse_released(button, x, y);
            }

            void lua_api::draw_mouse_dragged(api::ref_id id, int button, int x, int y)
            {
                auto dp = draw_refs.find(id);
                if(dp == draw_refs.end()) return;

                auto name = dp->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["b"] = button;
                    d["x"] = x;
                    d["y"] = y;
                    event_message em{name, "d", d, this};
                    send(em);
                }
                dp->second.mouse_dragged(button, x, y);
            }

            void lua_api::draw_mouse_moved(api::ref_id id, int x, int y)
            {
                auto dp = draw_refs.find(id);
                if(dp == draw_refs.end()) return;

                auto name = dp->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["x"] = x;
                    d["y"] = y;
                    event_message em{name, "m", d, this};
                    send(em);
                }

                dp->second.mouse_moved(x, y);
            }

            file_data_wrapper lua_api::open_file()
            {
                INVARIANT(front);
                file_data_wrapper w;
                w.file = front->open_file();
                return w;
            }

            bin_file_data_wrapper lua_api::open_bin_file()
            {
                INVARIANT(front);
                bin_file_data_wrapper w;
                w.file = front->open_bin_file();
                return w;
            }

            bool lua_api::save_file(const std::string& suggested_name, const std::string& data)
            {
                INVARIANT(front);
                return front->save_file(suggested_name, data);
            }

            bool lua_api::save_bin_file(const std::string& suggested_name, const bin_data& bin)
            {
                INVARIANT(front);
                return front->save_bin_file(suggested_name, bin.data);
            }

            void lua_api::button_clicked(api::ref_id id)
            {
                INVARIANT(state);

                std::string callback;
                std::string name;
                {
                    auto rp = button_refs.find(id);
                    if(rp == button_refs.end()) return;

                    callback = rp->second.callback;
                    name = rp->second.get_name();
                }
                if(callback.empty()) return;

                send_simple_event(name, "b");
                run(callback);
            }
        }
    }
}

