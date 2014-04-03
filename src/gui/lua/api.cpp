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
 */

#include <QtWidgets>

#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/env.hpp"
#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <QTimer>
#include <QTextBrowser>

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
            namespace
            {
                const std::string SANATIZE_REPLACE = "_";
                const size_t PADDING = 40;
            }

            lua_api::lua_api(
                    a::app_ptr a,
                    const us::contact_list& con,
                    ms::sender_ptr sndr,
                    s::conversation_ptr s,
                    s::conversation_service_ptr ss,
                    QWidget* c,
                    QGridLayout* cl,
                    list* o ) :
                app{a},
                contacts{con},
                sender{sndr},
                conversation{s},
                conversation_service{ss},
                canvas{c},
                layout{cl},
                output{o},
                ids{0},
                _error{}
            {
				_error.line = -1;
                INVARIANT(app);
                INVARIANT(sender);
                INVARIANT(conversation);
                INVARIANT(conversation_service);
                INVARIANT(canvas);
                INVARIANT(layout);

                local_data.reset(new store_ref{app->local_data()});
                data.reset(new store_ref{app->data()});

                bind();

                INVARIANT(canvas);
                INVARIANT(layout);
                INVARIANT(state);
                INVARIANT(local_data);
                INVARIANT(data);
            }

            lua_api::~lua_api()
            {
                reset_widgets();
            }

            QWidget* make_output_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            QWidget* make_error_widget(const std::string& text)
            {
                std::string m = "<b>error:</b> " + text; 
                auto tb = new QTextBrowser;
                tb->setHtml(m.c_str());
                return tb;
            }

            void lua_api::report_error(const std::string& e, int line)
            {
                _error.line = line;
                _error.message = e;

                LOG << "script error: " << e << std::endl;
                if(!output) return;
                output->add(make_error_widget(e));
            }

            error_info lua_api::get_error() const
            {
                return _error;
            }

            int lua_api::new_id()
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
                    .set("send", &lua_api::send_all)
                    .set("send_to", &lua_api::send_to)
                    .set("send_local", &lua_api::send_local)
                    .set("save_file", &lua_api::save_file)
                    .set("open_file", &lua_api::open_file)
                    .set("save_bin_file", &lua_api::save_bin_file)
                    .set("open_bin_file", &lua_api::open_bin_file)
                    .set("i_started", &lua_api::launched_local);

                SLB::Class<QPen>{"pen", &manager}
                    .set("set_width", &QPen::setWidth);

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
                    .set("get_bin", &script_message::get_bin)
                    .set("set_bin", &script_message::set_bin)
                    .set("is_local", &script_message::is_local)
                    .set("app", &script_message::app)
                    .set("set", script_message_set)
                    .set("get", script_message_get);

                SLB::Class<store_ref>{"store_ref", &manager}
                    .set("get_bin", &store_ref::get_bin)
                    .set("set_bin", &store_ref::set_bin)
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
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                SLB::Class<text_edit_ref>{"text_edit", &manager}
                    .set("text", &text_edit_ref::get_text)
                    .set("set_text", &text_edit_ref::set_text)
                    .set("edited_callback", &text_edit_ref::get_edited_callback)
                    .set("when_edited", &text_edit_ref::set_edited_callback)
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

                SLB::Class<bin_file_data>{"bin_file_data", &manager}
                    .set("good", &bin_file_data::is_good)
                    .set("name", &bin_file_data::get_name)
                    .set("size", &bin_file_data::get_size)
                    .set("data", &bin_file_data::get_data);

                SLB::Class<file_data>{"file_data", &manager}
                    .set("good", &file_data::is_good)
                    .set("name", &file_data::get_name)
                    .set("size", &file_data::get_size)
                    .set("data", &file_data::get_data);

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

            template <class L>
                void delete_loose_widgets(L& l)
                {
                    for(auto& w : l)
                    {
                        if(!w.second || w.second->parentWidget() != nullptr)
                            continue;
                        delete w.second;
                        w.second = 0;
                    }
                }

            void lua_api::reset_widgets()
            {
                INVARIANT(layout);
                std::lock_guard<std::mutex> lock(mutex);

                //delete widgets without parent
                delete_loose_widgets(widgets);

                //clear widgets
                QLayoutItem *c = 0;
                while((c = layout->takeAt(0)) != 0)
                {
                    if(!c || !c->widget()) continue;

                    delete c->widget();
                    delete c;
                } 

                for(auto& t : timers)
                {
                    if(t.second) 
                    {
                        t.second->stop();
                        delete t.second;
                    }
                }

                if(output) output->clear();
                button_refs.clear();
                edit_refs.clear();
                text_edit_refs.clear();
                list_refs.clear();
                timer_refs.clear();
                grid_refs.clear();
                image_refs.clear();
                widgets.clear();
                timers.clear();
                images.clear();

                ENSURE_EQUAL(layout->count(), 0);
            }

            void lua_api::run(const std::string& code)
            {
                REQUIRE_FALSE(code.empty());

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
                std::lock_guard<std::mutex> lock(mutex);
                INVARIANT(conversation);
                INVARIANT(conversation->user_service());

                if(!output) 
                {
                    LOG << "script: " << a << std::endl;
                    return;
                }

                auto self = conversation->user_service()->user().info().name();
                output->add(make_output_widget(self, a));
            }

            void lua_api::alert()
            {
                INVARIANT(conversation);
                INVARIANT(conversation_service);
                conversation_service->fire_conversation_alert(conversation->id());
            }

            void lua_api::message_received(const script_message& m)
            try
            {
                INVARIANT(state);

                if(m.is_local())
                {
                    if(local_message_callback.empty()) return;
                    state->call(local_message_callback, m);
                }
                else
                {
                    if(message_callback.empty()) return;
                    state->call(message_callback, m);
                }
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

            void lua_api::set_message_callback(const std::string& a)
            {
                message_callback = a;
            }

            void lua_api::set_local_message_callback(const std::string& a)
            {
                local_message_callback = a;
            }

            script_message lua_api::make_message()
            {
                return {this};
            }

            void lua_api::send_to_helper(us::user_info_ptr c, const script_message& m)
            {
                REQUIRE(c);
                if( !conversation->user_service()->contact_available(c->id()) || 
                    !conversation->contacts().has(c->id()))
                    return;

                sender->send(c->id(), m); 
            }

            void lua_api::send_all(const script_message& m)
            {
                INVARIANT(sender);
                for(auto c : contacts.list())
                {
                    CHECK(c);
                    send_to_helper(c, m);
                }
            }

            void lua_api::send_to(const contact_ref& cr, const script_message& m)
            {
                INVARIANT(sender);
                INVARIANT(conversation);

                auto c = contacts.by_id(cr.user_id);
                if(!c) return;
                send_to_helper(c, m);
            }

            size_t lua_api::total_contacts() const
            {
                return contacts.size();
            }

            int lua_api::last_contact() const
            {
                return contacts.size() - 1;
            }

            contact_ref lua_api::get_contact(size_t i)
            {
                auto c = contacts.get(i);
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
                    auto c = contacts.by_id(who_started_id);
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

                return conversation->app_ids().size();
            }

            app_ref lua_api::get_app(size_t i)
            {
                INVARIANT(conversation);
                const auto& ids = conversation->app_ids();
                if(i >= ids.size()) return empty_app_ref(*this);

                auto id = ids[i];

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
                for(const auto& id : conversation->app_ids())
                    sender->send_to_local_app(id, m);
            }

            grid_ref lua_api::make_grid()
            {
                INVARIANT(layout);
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create button reference
                grid_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create widget and new layout
                auto b = new QWidget;
                auto l = new QGridLayout;
                b->setLayout(l);

                //add ref and widget to maps
                grid_refs[ref.id] = ref;
                widgets[ref.id] = b;
                layouts[ref.id] = l;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::height(int h)
            {
                INVARIANT(canvas);
                canvas->setMinimumHeight(h);
            }

            void lua_api::grow()
            {
                INVARIANT(canvas);
                INVARIANT(layout);
                canvas->setMinimumHeight(layout->sizeHint().height() + PADDING);
            }

            void lua_api::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(layout);
                std::lock_guard<std::mutex> lock(mutex);

                auto w = get_widget<QWidget>(wr.id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c);
            }

            void lua_api::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(layout);
                std::lock_guard<std::mutex> lock(mutex);

                auto w = get_widget<QWidget>(wr.id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c, row_span, col_span);
            }

            button_ref lua_api::make_button(const std::string& text)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create button reference
                button_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create button widget
                auto b = new QPushButton(text.c_str());

                //map button to C++ callback
                auto mapper = new QSignalMapper{canvas};
                mapper->setMapping(b, ref.id);
                connect(b, SIGNAL(clicked()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(int)), this, SLOT(button_clicked(int)));

                //add ref and widget to maps
                button_refs[ref.id] = ref;
                widgets[ref.id] = b;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::button_clicked(int id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    auto rp = button_refs.find(id);
                    if(rp == button_refs.end()) return;

                    callback = rp->second.callback;
                }
                if(callback.empty()) return;

                run(callback);
            }

            label_ref lua_api::make_label(const std::string& text)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                label_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create edit widget
                auto w = new QLabel(text.c_str());

                //add ref and widget to maps
                label_refs[ref.id] = ref;
                widgets[ref.id] = w;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            edit_ref lua_api::make_edit(const std::string& text)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                edit_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create edit widget
                auto e = new QLineEdit(text.c_str());

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, ref.id);
                connect(e, SIGNAL(textChanged(QString)), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(int)), this, SLOT(edit_edited(int)));

                auto finished_mapper = new QSignalMapper{canvas};
                finished_mapper->setMapping(e, ref.id);
                connect(e, SIGNAL(editingFinished()), finished_mapper, SLOT(map()));
                connect(finished_mapper, SIGNAL(mapped(int)), this, SLOT(edit_finished(int)));

                //add ref and widget to maps
                edit_refs[ref.id] = ref;
                widgets[ref.id] = e;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.finished_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::edit_edited(int id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    auto rp = edit_refs.find(id);
                    if(rp == edit_refs.end()) return;

                    callback = rp->second.edited_callback;
                }
                if(callback.empty()) return;

                run(callback);
            }

            void lua_api::edit_finished(int id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    auto rp = edit_refs.find(id);
                    if(rp == edit_refs.end()) return;

                    callback = rp->second.finished_callback;
                }
                if(callback.empty()) return;

                run(callback);
            }

            text_edit_ref lua_api::make_text_edit(const std::string& text)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                text_edit_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create edit widget
                auto e = new QTextEdit(text.c_str());

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, ref.id);
                connect(e, SIGNAL(textChanged()), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(int)), this, SLOT(text_edit_edited(int)));

                //add ref and widget to maps
                text_edit_refs[ref.id] = ref;
                widgets[ref.id] = e;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::text_edit_edited(int id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    auto rp = text_edit_refs.find(id);
                    if(rp == text_edit_refs.end()) return;

                    callback = rp->second.edited_callback;
                }
                if(callback.empty()) return;

                run(callback);
            }

            list_ref lua_api::make_list()
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                list_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create edit widget
                auto w = new gui::list;
                w->auto_scroll(true);

                //add ref and widget to maps
                list_refs[ref.id] = ref;
                widgets[ref.id] = w;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            QPen lua_api::make_pen(const std::string& color, int width)
            try
            {
                std::lock_guard<std::mutex> lock(mutex);
                QPen p{QColor{color.c_str()}};
                p.setWidth(width);
                return p;
            }
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in make_pen: " << e.what();
                report_error(s.str());
            }
            catch(...)
            {
                report_error("error in make_pen: unknown");
            }

            draw_ref lua_api::make_draw(int width, int height)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                draw_ref ref;
                ref.id = new_id();
                ref.api = this;

                //create edit widget
                auto w = new draw_view{ref, width, height};

                //add ref and widget to maps
                draw_refs[ref.id] = ref;
                widgets[ref.id] = w;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            timer_ref lua_api::make_timer(int msec, const std::string& callback)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);

                //create edit reference
                timer_ref ref;
                ref.id = new_id();
                ref.api = this;
                ref.callback = callback;

                //create timer 
                auto t = new QTimer{canvas};

                //map timer to C++ callback
                auto timer_mapper = new QSignalMapper{canvas};
                timer_mapper->setMapping(t, ref.id);

                connect(t, SIGNAL(timeout()), timer_mapper, SLOT(map()));
                connect(timer_mapper, SIGNAL(mapped(int)), this, SLOT(timer_triggered(int)));

                //add ref and widget to maps
                timer_refs[ref.id] = ref;
                timers[ref.id] = t;

                //start timer
                t->start(msec);

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            void lua_api::timer_triggered(int id)
            {
                INVARIANT(state);

                std::string callback;
                {
                    std::lock_guard<std::mutex> lock(mutex);

                    auto t = timer_refs.find(id);
                    if(t == timer_refs.end()) return;
                    callback = t->second.callback;
                }

                if(callback.empty()) return;
                run(callback);
            }

            image_ref lua_api::make_image(const bin_data& d)
            {
                INVARIANT(canvas);
                std::lock_guard<std::mutex> lock(mutex);
                
                //create edit reference
                image_ref ref;
                ref.id = new_id();
                ref.api = this;
                ref.w = 0;
                ref.h = 0;
                ref.g = false;

                auto i = std::make_shared<QImage>();
                bool loaded = i->loadFromData(reinterpret_cast<const u::ubyte*>(d.data.data()),d.data.size());

                auto l = new QLabel;
                if(loaded) 
                {
                    ref.w = i->width();
                    ref.h = i->height();
                    ref.g = true;
                    l->setPixmap(QPixmap::fromImage(*i));
                    l->setMinimumSize(i->width(), i->height());
                }

                image_refs[ref.id] = ref;
                widgets[ref.id] = l;
                images[ref.id] = i;

                ENSURE_FALSE(ref.id == 0);
                ENSURE(ref.api);
                return ref;
            }

            file_data lua_api::open_file()
            {
                INVARIANT(canvas);

                auto sf = get_file_name(canvas);
                if(sf.empty()) return file_data{};
                bf::path p = sf;

                std::ifstream f(sf.c_str());
                if(!f) return file_data{};

                std::string data{std::istream_iterator<char>(f), std::istream_iterator<char>()};

                file_data fd;
                fd.name = p.filename().string();
                fd.data = std::move(data);
                fd.good = true;
                LOG << "opened file `" << fd.name << " size " << fd.data.size() << std::endl;
                return fd;
            }

            bin_file_data lua_api::open_bin_file()
            {
                INVARIANT(canvas);
                auto sf = get_file_name(canvas);
                if(sf.empty()) return bin_file_data{};
                bf::path p = sf;

                bin_data bin;
                if(!load_from_file(sf, bin.data)) return bin_file_data{};

                bin_file_data fd;
                fd.name = p.filename().string();
                fd.data = std::move(bin);
                fd.good = true;
                LOG << "opened bin file `" << fd.name << " size " << fd.data.data.size() << std::endl;
                return fd;
            }

            std::string sanatize(const std::string& s)
            {
                const boost::regex SANATIZE_PATH_REGEX("[\\\\\\/\\:]");  //matches \, /, and :
                return boost::regex_replace (s, SANATIZE_PATH_REGEX , SANATIZE_REPLACE);
            }

            bool lua_api::save_file(const std::string& suggested_name, const std::string& data)
            {
                if(data.empty()) return false;
                auto home = u::get_home_dir();
                std::string suggested_path = home + "/" + sanatize(suggested_name);
                auto file = QFileDialog::getSaveFileName(canvas, tr("Save File"), suggested_path.c_str());
                if(file.isEmpty()) return false;

                auto fs = convert(file);
                std::ofstream o(fs.c_str());
                if(!o) return false;

                o.write(data.c_str(), data.size());
                LOG << "saved: " << fs << " size " << data.size() <<  std::endl;
                return true;
            }

            bool lua_api::save_bin_file(const std::string& suggested_name, const bin_data& bin)
            {
                if(bin.data.empty()) return false;
                auto home = u::get_home_dir();
                std::string suggested_path = home + "/" + sanatize(suggested_name);
                auto file = QFileDialog::getSaveFileName(canvas, tr("Save File"), suggested_path.c_str());
                if(file.isEmpty()) return false;

                auto fs = convert(file);
                std::ofstream o(fs.c_str(), std::fstream::out | std::fstream::binary);
                if(!o) return false;

                o.write(bin.data.data(), bin.data.size());
                LOG << "saved bin: " << fs << " size " << bin.data.size() << std::endl;
                return true;
            }

        }
    }
}

