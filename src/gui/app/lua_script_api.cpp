/*
 * Copyright (C) 2012  Maxim Noah Khailo
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
            const std::string SCRIPT_MESSAGE = "script_msg";
            script_message::script_message(lua_script_api* api) : 
                _from_id{}, _v{}, _api{api} 
                { 
                    REQUIRE(_api);
                    INVARIANT(_api);
                }

            script_message::script_message(const m::message& m, lua_script_api* api) :
                _api{api}
            {
                REQUIRE(api);
                REQUIRE_EQUAL(m.meta.type, SCRIPT_MESSAGE);

                _from_id = m.meta.extra["from_id"].as_string();
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
                return _v[k].as_string();
            }

            void script_message::set(const std::string& k, const std::string& v) 
            {
                _v[k] = v;
            }

            contact_ref empty_contact_ref(lua_script_api& api)
            {
                contact_ref e;
                e.api = &api;
                e.id = "0";
                return e;
            }

            contact_ref script_message::from() const
            {
                INVARIANT(_api);

                auto c = _api->contacts.by_id(_from_id);
                if(!c) return empty_contact_ref(*_api);

                contact_ref r;
                r.id = c->id();
                r.api = _api;

                ENSURE_EQUAL(r.api, _api);
                ENSURE_FALSE(r.id.empty());
                return r;
            }

            lua_script_api::lua_script_api(
                    const us::contact_list& con,
                    ms::sender_ptr sndr,
                    s::session_ptr s,
                    QWidget* c,
                    QGridLayout* cl,
                    list* o ) :
                contacts{con},
                sender{sndr},
                session{s},
                canvas{c},
                layout{cl},
                output{o}
            {
                INVARIANT(sender);
                INVARIANT(session);
                INVARIANT(canvas);
                INVARIANT(layout);

                bind();

                INVARIANT(canvas);
                INVARIANT(layout);
                INVARIANT(state);
            }

            void lua_script_api::bind()
            {  
                REQUIRE_FALSE(state);

                using namespace std::placeholders;

                SLB::Class<lua_script_api, SLB::Instance::NoCopyNoDestroy>{"Api", &manager}
                    .set("print", &lua_script_api::print)
                    .set("button", &lua_script_api::make_button)
                    .set("label", &lua_script_api::make_label)
                    .set("edit", &lua_script_api::make_edit)
                    .set("text_edit", &lua_script_api::make_text_edit)
                    .set("list", &lua_script_api::make_list)
                    .set("canvas", &lua_script_api::make_canvas)
                    .set("place", &lua_script_api::place)
                    .set("place_across", &lua_script_api::place_across)
                    .set("total_contacts", &lua_script_api::total_contacts)
                    .set("last_contact", &lua_script_api::last_contact)
                    .set("contact", &lua_script_api::get_contact)
                    .set("message", &lua_script_api::make_message)
                    .set("when_message_received", &lua_script_api::set_message_callback)
                    .set("send", &lua_script_api::send_all)
                    .set("send_to", &lua_script_api::send_to);

                SLB::Class<contact_ref>{"contact", &manager}
                    .set("name", &contact_ref::get_name)
                    .set("online", &contact_ref::is_online);

                SLB::Class<script_message>{"script_message", &manager}
                    .set("from", &script_message::from)
                    .set("get", &script_message::get)
                    .set("set", &script_message::set);

                SLB::Class<canvas_ref>{"canvas", &manager}
                    .set("place", &canvas_ref::place)
                    .set("place_across", &canvas_ref::place_across);

                SLB::Class<button_ref>{"button", &manager}
                    .set("text", &button_ref::get_text)
                    .set("set_text", &button_ref::set_text)
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
                    .set("clear", &list_ref::clear)
                    .set("enabled", &widget_ref::enabled)
                    .set("enable", &widget_ref::enable)
                    .set("disable", &widget_ref::disable);

                state = std::make_shared<SLB::Script>(&manager);
                state->set("app", this);

                ENSURE(state);
            }

            QWidget* make_output_widget(const std::string& name, const std::string& text)
            {
                std::string m = "<b>" + name + "</b>: " + text; 
                return new QLabel{m.c_str()};
            }

            QWidget* make_error_widget(const std::string& text)
            {
                std::string m = "<b>error:</b> " + text; 
                return new QLabel{m.c_str()};
            }

            std::string lua_script_api::execute(const std::string& s)
            {
                REQUIRE_FALSE(s.empty());
                INVARIANT(state);

                return state->safeDoString(s.c_str()) ? "" : state->getLastError();
            }

            void lua_script_api::reset_widgets()
            {
                INVARIANT(layout);

                //clear widgets
                QLayoutItem *c = 0;
                while((c = layout->takeAt(0)) != 0)
                {
                    CHECK(c);
                    CHECK(c->widget());

                    delete c->widget();
                    delete c;
                } 

                if(output) output->clear();
                button_refs.clear();
                edit_refs.clear();
                text_edit_refs.clear();
                list_refs.clear();
                canvas_refs.clear();
                widgets.clear();

                ENSURE_EQUAL(layout->count(), 0);
            }

            void lua_script_api::run(const std::string& code)
            {
                REQUIRE_FALSE(code.empty());

                auto error = execute(code);
                if(!error.empty() && output) output->add(make_error_widget("error: " + error));
            }

            //API implementation 
            template<class W>
                W* get_widget(const std::string& id, widget_map& map)
                {
                    auto wp = map.find(id);
                    return wp != map.end() ? dynamic_cast<W*>(wp->second) : nullptr;
                }

            QGridLayout* get_layout(const std::string& id, layout_map& map)
            {
                auto lp = map.find(id);
                return lp != map.end() ? lp->second : nullptr;
            }

            void set_enabled(const std::string& id, widget_map& map, bool enabled)
            {
                auto w = get_widget<QWidget>(id, map);
                if(!w) return;

                w->setEnabled(enabled);
            }

            void lua_script_api::print(const std::string& a)
            {
                INVARIANT(session);
                INVARIANT(session->user_service());

                if(!output) return;

                auto self = session->user_service()->user().info().name();
                output->add(make_output_widget(self, a));
            }

            void lua_script_api::message_recieved(const script_message& m)
            try
            {
                INVARIANT(state);
                if(message_callback.empty()) return;

                state->call(message_callback, m);
            }
            catch(std::exception& e)
            {
                if(!output) return;

                std::stringstream s;
                s << "error in message callback: " << e.what();
                output->add(make_error_widget(s.str()));
            }
            catch(...)
            {
                if(!output) return;
                output->add(make_error_widget("error in message callback: unknown"));
            }

            void lua_script_api::set_message_callback(const std::string& a)
            {
                message_callback = a;
            }

            script_message lua_script_api::make_message()
            {
                return {this};
            }

            void lua_script_api::send_all(const script_message& m)
            {
                INVARIANT(sender);
                for(auto c : contacts.list())
                {
                    CHECK(c);
                    sender->send(c->id(), m); 
                }
            }

            void lua_script_api::send_to(const contact_ref& cr, const script_message& m)
            {
                INVARIANT(sender);

                auto c = contacts.by_id(cr.id);
                if(!c) return;

                sender->send(c->id(), m); 
            }

            size_t lua_script_api::total_contacts() const
            {
                return contacts.size();
            }

            int lua_script_api::last_contact() const
            {
                return contacts.size() - 1;
            }

            contact_ref lua_script_api::get_contact(size_t i)
            {
                auto c = contacts.get(i);
                if(!c) return empty_contact_ref(*this);

                contact_ref r;
                r.id = c->id();
                r.api = this;

                ENSURE_EQUAL(r.api, this);
                ENSURE_FALSE(r.id.empty());
                return r;
            }

            std::string contact_ref::get_name() const
            {
                INVARIANT(api);

                auto c = api->contacts.by_id(id);
                if(!c) return "";

                return c->name();
            }

            bool contact_ref::is_online() const
            {
                INVARIANT(api);
                INVARIANT(api->session);

                return api->session->user_service()->contact_available(id);
            }

            canvas_ref lua_script_api::make_canvas(int r, int c)
            {
                INVARIANT(layout);
                INVARIANT(canvas);

                //create button reference
                canvas_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create widget and new layout
                auto b = new QWidget;
                auto l = new QGridLayout;
                b->setLayout(l);

                //add ref and widget to maps
                canvas_refs[ref.id] = ref;
                widgets[ref.id] = b;
                layouts[ref.id] = l;

                //place
                layout->addWidget(b, r, c);

                ENSURE_FALSE(ref.id.empty());
                ENSURE(ref.api);
                return ref;
            }

            void canvas_ref::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id.empty());

                auto l = get_layout(id, api->layouts);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

                l->addWidget(w, r, c);
            }

            void canvas_ref::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id.empty());

                auto l = get_layout(id, api->layouts);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

                l->addWidget(w, r, c, row_span, col_span);
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

            void lua_script_api::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(layout);

                auto w = get_widget<QWidget>(wr.id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c);
            }

            void lua_script_api::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(layout);

                auto w = get_widget<QWidget>(wr.id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c, row_span, col_span);
            }

            button_ref lua_script_api::make_button(const std::string& text)
            {
                INVARIANT(canvas);

                //create button reference
                button_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create button widget
                auto b = new QPushButton(text.c_str());

                //map button to C++ callback
                auto mapper = new QSignalMapper{canvas};
                mapper->setMapping(b, QString(ref.id.c_str()));
                connect(b, SIGNAL(clicked()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(QString)), this, SLOT(button_clicked(QString)));

                //add ref and widget to maps
                button_refs[ref.id] = ref;
                widgets[ref.id] = b;

                ENSURE_FALSE(ref.id.empty());
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

                run(callback);
            }

            std::string button_ref::get_text() const
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return "";

                auto button = get_widget<QPushButton>(id, api->widgets);
                CHECK(button);

                return gui::convert(button->text());
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget<QPushButton>(id, api->widgets);
                CHECK(button);

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

            label_ref lua_script_api::make_label(const std::string& text)
            {
                INVARIANT(canvas);

                //create edit reference
                label_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create edit widget
                auto w = new QLabel(text.c_str());

                //add ref and widget to maps
                label_refs[ref.id] = ref;
                widgets[ref.id] = w;

                ENSURE_FALSE(ref.id.empty());
                ENSURE(ref.api);
                return ref;
            }

            std::string label_ref::get_text() const
            {
                INVARIANT(api);

                auto rp = api->label_refs.find(id);
                if(rp == api->label_refs.end()) return "";

                auto l = get_widget<QLabel>(id, api->widgets);
                CHECK(l);

                return gui::convert(l->text());
            }

            void label_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->label_refs.find(id);
                if(rp == api->label_refs.end()) return;

                auto l = get_widget<QLabel>(id, api->widgets);
                CHECK(l);

                l->setText(t.c_str());
            }

            edit_ref lua_script_api::make_edit(const std::string& text)
            {
                INVARIANT(canvas);

                //create edit reference
                edit_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create edit widget
                auto e = new QLineEdit(text.c_str());

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, QString(ref.id.c_str()));
                connect(e, SIGNAL(textChanged(QString)), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(QString)), this, SLOT(edit_edited(QString)));

                auto finished_mapper = new QSignalMapper{canvas};
                finished_mapper->setMapping(e, QString(ref.id.c_str()));
                connect(e, SIGNAL(editingFinished()), finished_mapper, SLOT(map()));
                connect(finished_mapper, SIGNAL(mapped(QString)), this, SLOT(edit_finished(QString)));

                //add ref and widget to maps
                edit_refs[ref.id] = ref;
                widgets[ref.id] = e;

                ENSURE_FALSE(ref.id.empty());
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.finished_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_script_api::edit_edited(QString id)
            {
                INVARIANT(state);

                auto rp = edit_refs.find(gui::convert(id));
                if(rp == edit_refs.end()) return;

                const auto& callback = rp->second.edited_callback;
                if(callback.empty()) return;

                run(callback);
            }

            void lua_script_api::edit_finished(QString id)
            {
                INVARIANT(state);

                auto rp = edit_refs.find(gui::convert(id));
                if(rp == edit_refs.end()) return;

                const auto& callback = rp->second.finished_callback;
                if(callback.empty()) return;

                run(callback);
            }

            std::string edit_ref::get_text() const
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return "";

                auto edit = get_widget<QLineEdit>(id, api->widgets);
                CHECK(edit);

                return gui::convert(edit->text());
            }

            void edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                auto edit = get_widget<QLineEdit>(id, api->widgets);
                CHECK(edit);

                edit->setText(t.c_str());
            }

            void edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            void edit_ref::set_finished_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.finished_callback = c;
                finished_callback = c;
            }

            text_edit_ref lua_script_api::make_text_edit(const std::string& text)
            {
                INVARIANT(canvas);

                //create edit reference
                text_edit_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create edit widget
                auto e = new QTextEdit(text.c_str());

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, QString(ref.id.c_str()));
                connect(e, SIGNAL(textChanged()), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(QString)), this, SLOT(text_edit_edited(QString)));

                //add ref and widget to maps
                text_edit_refs[ref.id] = ref;
                widgets[ref.id] = e;

                ENSURE_FALSE(ref.id.empty());
                ENSURE(ref.edited_callback.empty());
                ENSURE(ref.api);
                return ref;
            }

            void lua_script_api::text_edit_edited(QString id)
            {
                INVARIANT(state);

                auto rp = text_edit_refs.find(gui::convert(id));
                if(rp == text_edit_refs.end()) return;

                const auto& callback = rp->second.edited_callback;
                if(callback.empty()) return;

                run(callback);
            }

            std::string text_edit_ref::get_text() const
            {
                INVARIANT(api);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return "";

                auto edit = get_widget<QTextEdit>(id, api->widgets);
                CHECK(edit);

                return gui::convert(edit->toPlainText());
            }

            void text_edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                auto edit = get_widget<QTextEdit>(id, api->widgets);
                CHECK(edit);

                edit->setText(t.c_str());
            }

            void text_edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            list_ref lua_script_api::make_list()
            {
                INVARIANT(canvas);

                //create edit reference
                list_ref ref;
                ref.id = u::uuid();
                ref.api = this;

                //create edit widget
                auto w = new gui::list;
                w->auto_scroll(true);

                //add ref and widget to maps
                list_refs[ref.id] = ref;
                widgets[ref.id] = w;

                ENSURE_FALSE(ref.id.empty());
                ENSURE(ref.api);
                return ref;
            }

            void list_ref::add(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id.empty());
                INVARIANT(api);
                INVARIANT_FALSE(id.empty());

                auto l = get_widget<gui::list>(id, api->widgets);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

                l->add(w);
            }

            void list_ref::clear()
            {
                INVARIANT(api);
                INVARIANT_FALSE(id.empty());

                auto l = get_widget<gui::list>(id, api->widgets);
                if(!l) return;

                l->clear();
            }
        }
    }
}

