/*
 * Copyright (C) 2012  Maxim Noah Khailo
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

#include <QtGui>

#include "gui/app/lua_script_api.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"

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
        namespace app
        {
            lua_script_api::lua_script_api(
                    ms::sender_ptr sndr,
                    s::session_ptr s) :
                sender{sndr},
                session{s}
            {
                INVARIANT(sender);
                INVARIANT(session);

                //canvas
                canvas = new QWidget;
                layout = new QGridLayout;
                canvas->setLayout(layout);

                //init mappers
                button_mapper = new QSignalMapper(canvas);
                edit_text_edited_mapper = new QSignalMapper(canvas);
                edit_finished_mapper = new QSignalMapper(canvas);

                //message list
                output = new list;

                bind();

                INVARIANT(canvas);
                INVARIANT(layout);
                INVARIANT(button_mapper);
                INVARIANT(edit_text_edited_mapper);
                INVARIANT(edit_finished_mapper);
                INVARIANT(output);
                INVARIANT(state);
            }

            void lua_script_api::bind()
            {  
                REQUIRE_FALSE(state);

                using namespace std::placeholders;
                SLB::Class<lua_script_api, SLB::Instance::NoCopyNoDestroy>{"Api", &manager}
                    .set("print", &lua_script_api::print)
                    .set("button", &lua_script_api::make_button)
                    .set("edit", &lua_script_api::make_edit);

                SLB::Class<button_ref>{"button", &manager}
                    .set("get_text", &button_ref::get_text)
                    .set("set_text", &button_ref::set_text)
                    .set("get_callback", &button_ref::get_callback)
                    .set("when_clicked", &button_ref::set_callback)
                    .set("enabled", &button_ref::enabled)
                    .set("enable", &button_ref::enable)
                    .set("disable", &button_ref::disable);

                SLB::Class<edit_ref>{"edit", &manager}
                    .set("get_text", &edit_ref::get_text)
                    .set("set_text", &edit_ref::set_text)
                    .set("get_edited_callback", &edit_ref::get_text_edited_callback)
                    .set("when_edited", &edit_ref::set_text_edited_callback)
                    .set("get_finished_callback", &edit_ref::get_finished_callback)
                    .set("when_finished", &edit_ref::set_finished_callback)
                    .set("enabled", &edit_ref::enabled)
                    .set("enable", &edit_ref::enable)
                    .set("disable", &edit_ref::disable);

                state.reset(new SLB::Script{&manager});
                state->set("str", this);

                ENSURE(state);
            }

            QWidget* make_output_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            std::string lua_script_api::execute(const std::string& s)
            try
            {
                REQUIRE_FALSE(s.empty());
                INVARIANT(state);

                state->doString(s.c_str());
                return "";
            }
            catch(std::exception& e)
            {
                return e.what();
            }
            catch(...)
            {
                return "unknown";
            }

            void lua_script_api::run(const std::string name, const std::string& code)
            {
                INVARIANT(output);
                REQUIRE_FALSE(code.empty());

                output->add(make_output_widget(name, "code: " + code));
                auto error = execute(code);
                if(!error.empty()) output->add(make_output_widget(name, "error: " + error));
            }


            //API implementation 
            template<class M>
                typename M::mapped_type get_widget(const std::string& id, M& map)
                {
                    auto wp = map.find(id);
                    return wp != map.end() ? wp->second : nullptr;
                }

            template<class widget_ref, class M>
                void set_enabled(widget_ref& wr, M& map, bool enabled)
                {
                    auto w = get_widget(wr.id, map);
                    if(!w) return;

                    w->setEnabled(enabled);
                }

            void lua_script_api::print(const std::string& a)
            {
                INVARIANT(session);
                INVARIANT(output);
                INVARIANT(session->user_service());

                auto self = session->user_service()->user().info().name();
                output->add(make_output_widget(self, a));
            }

            button_ref lua_script_api::make_button(const std::string& text, int r, int c)
            {
                INVARIANT(layout);
                INVARIANT(button_mapper);

                //create button reference
                button_ref ref{u::uuid(), text, "", this};

                //create button widget
                auto b = new QPushButton(text.c_str());
                layout->addWidget(b, r, c);

                //map button to C++ callback
                button_mapper->setMapping(b, QString(ref.id.c_str()));
                connect(b, SIGNAL(clicked()), button_mapper, SLOT(map()));
                connect(button_mapper, SIGNAL(mapped(QString)), this, SLOT(button_clicked(QString)));

                //add ref and widget to maps
                button_refs[ref.id] = ref;
                button_widgets[ref.id] = b;

                ENSURE_EQUAL(ref.text, text);
                ENSURE(ref.callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_script_api::button_clicked(QString id)
            {
                INVARIANT(state);

                auto rp = button_refs.find(gui::convert(id));
                if(rp == button_refs.end()) return;

                const auto& callback = rp->second.callback;
                if(callback.empty()) return;

                state->call(callback);
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget(id, api->button_widgets);
                CHECK(button);

                rp->second.text = t;
                text = t;
                button->setText(t.c_str());
            }

            void button_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                rp->second.callback = c;
                callback = c;
            }  

            bool button_ref::enabled()
            {
                INVARIANT(api);
                auto button = get_widget(id, api->button_widgets);
                return button ? button->isEnabled() : false;
            }

            void button_ref::enable()
            {
                INVARIANT(api);

                set_enabled(*this, api->button_widgets, true);
            }

            void button_ref::disable()
            {
                INVARIANT(api);

                set_enabled(*this, api->button_widgets, false);
            }

            edit_ref lua_script_api::make_edit(const std::string& text, int r, int c)
            {
                INVARIANT(layout);
                INVARIANT(edit_text_edited_mapper);
                INVARIANT(edit_finished_mapper);

                //create edit reference
                std::string text_edited_callback = "";
                std::string finished_callback = "";
                edit_ref ref{u::uuid(), text, text_edited_callback, finished_callback, this};

                //create edit widget
                auto e = new QLineEdit(text.c_str());
                layout->addWidget(e, r, c);

                //map edit to C++ callback
                edit_text_edited_mapper->setMapping(e, QString(ref.id.c_str()));
                connect(e, SIGNAL(textChanged(QString)), edit_text_edited_mapper, SLOT(map()));
                connect(edit_text_edited_mapper, SIGNAL(mapped(QString)), this, SLOT(edit_text_edited(QString)));

                edit_finished_mapper->setMapping(e, QString(ref.id.c_str()));
                connect(e, SIGNAL(editingFinished()), edit_finished_mapper, SLOT(map()));
                connect(edit_finished_mapper, SIGNAL(mapped(QString)), this, SLOT(edit_finished(QString)));

                //add ref and widget to maps
                edit_refs[ref.id] = ref;
                edit_widgets[ref.id] = e;

                ENSURE_EQUAL(ref.text, text);
                ENSURE(ref.text_edited_callback.empty());
                ENSURE(ref.finished_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_script_api::edit_text_edited(QString id)
            {
                INVARIANT(state);

                auto rp = edit_refs.find(gui::convert(id));
                if(rp == edit_refs.end()) return;

                const auto& callback = rp->second.text_edited_callback;
                if(callback.empty()) return;

                state->call(callback);
            }

            void lua_script_api::edit_finished(QString id)
            {
                INVARIANT(state);

                auto rp = edit_refs.find(gui::convert(id));
                if(rp == edit_refs.end()) return;

                const auto& callback = rp->second.finished_callback;
                if(callback.empty()) return;

                state->call(callback);
            }

            void edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                auto edit = get_widget(id, api->edit_widgets);
                CHECK(edit);

                rp->second.text = t;
                text = t;
                edit->setText(t.c_str());
            }

            void edit_ref::set_text_edited_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.text_edited_callback = c;
                text_edited_callback = c;
            }

            void edit_ref::set_finished_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.finished_callback = c;
                finished_callback = c;
            }

            bool edit_ref::enabled()
            {
                INVARIANT(api);
                auto edit = get_widget(id, api->edit_widgets);
                return edit ? edit->isEnabled() : false;
            }

            void edit_ref::enable()
            {
                INVARIANT(api);

                set_enabled(*this, api->edit_widgets, true);
            }

            void edit_ref::disable()
            {
                INVARIANT(api);

                set_enabled(*this, api->edit_widgets, false);
            }
        }
    }
}

