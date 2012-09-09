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

                button_mapper = new QSignalMapper(canvas);

                //message list
                output = new list;

                bind();

                INVARIANT(canvas);
                INVARIANT(layout);
                INVARIANT(button_mapper);
                INVARIANT(output);
                INVARIANT(state);
            }

            void lua_script_api::bind()
            {  
                REQUIRE_FALSE(state);

                using namespace std::placeholders;
                SLB::Class<lua_script_api, SLB::Instance::NoCopyNoDestroy>{"Api", &manager}
                    .set("print", &lua_script_api::print)
                    .set("button", &lua_script_api::button);

                SLB::Class<button_ref>{"button", &manager}
                    .set("get_text", &button_ref::get_text)
                    .set("set_text", &button_ref::set_text)
                    .set("get_callback", &button_ref::get_callback)
                    .set("set_callback", &button_ref::set_callback)
                    .set("enabled", &button_ref::enabled)
                    .set("enable", &button_ref::enable)
                    .set("disable", &button_ref::disable);

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
            void lua_script_api::print(const std::string& a)
            {
                INVARIANT(session);
                INVARIANT(output);
                INVARIANT(session->user_service());

                auto self = session->user_service()->user().info().name();
                output->add(make_output_widget(self, a));
            }

            button_ref lua_script_api::button(const std::string& text, const std::string& callback, int r, int c)
            {
                INVARIANT(layout);
                INVARIANT(button_mapper);

                //create button reference
                button_ref ref{u::uuid(), text, callback, this};

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
                ENSURE_EQUAL(ref.callback, callback);
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

                state->call(rp->second.callback);
            }

            QPushButton* get_widget(const button_ref& r, lua_script_api& api)
            {
                auto wp = api.button_widgets.find(r.id);
                return wp != api.button_widgets.end() ? wp->second : nullptr;
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget(*this, *api);
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

                auto button = get_widget(*this, *api);
                if(!button) return false;

                return button->isEnabled();
            }

            void button_ref::enable()
            {
                INVARIANT(api);

                auto button = get_widget(*this, *api);
                if(!button) return;

                button->setEnabled(true);
            }

            void button_ref::disable()
            {
                INVARIANT(api);

                auto button = get_widget(*this, *api);
                if(!button) return;

                button->setEnabled(false);
            }
        }
    }
}

