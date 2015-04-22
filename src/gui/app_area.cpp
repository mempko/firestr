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

#include <QtWidgets>

#include "gui/app_area.hpp"
#include "gui/unknown_message.hpp"
#include "gui/app/chat.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <sstream>
#include <algorithm>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace a = fire::gui::app;
namespace u = fire::util;
namespace fg = fire::gui;


namespace fire
{
    namespace gui
    {
        app_area::app_area(
                a::app_service_ptr app_service,
                a::app_reaper_ptr app_reaper,
                s::conversation_service_ptr conversation_s,
                s::conversation_ptr conversation) :
            QMdiArea{nullptr},
            _conversation_service{conversation_s},
            _conversation{conversation},
            _app_service{app_service},
            _app_reaper{app_reaper}
        {
            REQUIRE(app_service);
            REQUIRE(app_reaper);
            REQUIRE(conversation_s);
            REQUIRE(conversation);

            setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

            connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(clear_alerts()));
            connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(clear_alerts()));
            connect(this, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(sub_window_activated(QMdiSubWindow*)));


            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);
        }

        size_t count_open(const QList<QMdiSubWindow *>& l)
        {
            return std::count_if(l.begin(), l.end(), 
                    [](const QMdiSubWindow* w) 
                    { 
                        REQUIRE(w); 
                        if(dynamic_cast<app::app_editor*>(w->widget()) != nullptr) return false;
                        return w->visibleRegion().isEmpty() && !w->isMinimized();
                    });
        }

        /**
         * keep open the last 'op' amount of windows. 
         * This is so that when a conversation is created in the background and apps are loaded
         * into it, it won't be overwhelming to the user.
         */
        void keep_open(QList<QMdiSubWindow *> l, size_t op)
        {
            std::reverse(l.begin(), l.end());
            size_t c = 0;
            for(auto sw :  l)
            {
                CHECK(sw);
                if(!sw->isMinimized())
                {
                    c++;
                    if(c > op && sw->visibleRegion().isEmpty())
                        sw->showMinimized();
                }
            }
        }

        void app_area::handle_resize_hack()
        {
            auto l = subWindowList();
            auto o = count_open(l);
            if(o > 2) return;

            keep_open(l, 1);
        }

        void app_area::add(app::generic_app* m)
        {
            REQUIRE(m);

            connect(m, SIGNAL(did_resize_hack()), this, SLOT(handle_resize_hack()));

            auto sw = addSubWindow(m);
            CHECK(sw);

            m->set_sub_window(sw);

            Qt::WindowFlags flags = Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowTitleHint;
            sw->setWindowFlags(flags);
            sw->setWindowTitle(m->title_text().c_str());

            ENSURE(sw->widget() == m);

            //start generic app
            m->start();
            m->set_alert();
        }

        s::conversation_ptr app_area::conversation()
        {
            ENSURE(_conversation);
            return _conversation;
        }

        a::app_service_ptr app_area::app_service()
        {
            ENSURE(_app_service);
            return _app_service;
        }

        bool app_area::add_new_app(const ms::new_app& n) 
        {
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);

            a::app_ptr app;
            s::app_metadatum m;
            m.type = n.type();
            m.address = n.id();

            if(_conversation->has_app(m.address)) return false;
            if(m.type.empty() || m.address.empty()) return false;

            a::generic_app* c = nullptr;

            if(n.type() == a::CHAT)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    c = new a::chat_app{n.id(), _conversation_service, _conversation};
                    CHECK(c->mail());

                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::APP_EDITOR)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    app = _app_service->create_new_app();
                    app->launched_local(false);
                    c = new a::app_editor{
                        n.from_id(), n.id(), 
                            _app_service, _app_reaper, 
                            _conversation_service, _conversation, app};
                    CHECK(c->mail());

                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::SCRIPT_APP)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    app = _app_service->create_app(u::decode<m::message>(n.data()));
                    if(app->id().empty()) return false;

                    c = new a::script_app{
                        n.from_id(), n.id(), app, 
                            _app_service, _app_reaper, 
                            _conversation_service, _conversation};
                    CHECK(c->mail());

                    m.id = app->id();
                    post->add(c->mail());
                    add(c);
                }
            }
            else
            {
                LOG << "unknown app type`" << n.type() << "'" << std::endl;
                return false;
            }

            _conversation->add_app(m);

            CHECK(c);
            app_pair p{app, c};
            _apps[m.address] = p;

            return true;
        }

        void app_area::add_chat_app()
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            //create chat app
            auto t = new a::chat_app{_conversation_service, _conversation};
            add(t, nullptr, ""); 
        }

        void app_area::add_app_editor(const std::string& id)
        {
            INVARIANT(_app_service)
            INVARIANT(_app_reaper)
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            a::app_ptr app = id.empty() ? 
                _app_service->create_new_app() : 
                _app_service->load_app(id);

            CHECK(app);

            //create app editor
            auto t = new a::app_editor{_app_service, _app_reaper, _conversation_service, _conversation, app};
            add(t, nullptr, ""); 
        }

        void app_area::add_script_app(const std::string& id)
        {
            INVARIANT(_app_service)
            INVARIANT(_app_reaper)
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            //load app
            auto a = _app_service->load_app(id);
            if(!a) return;

            //create script app
            auto t = new a::script_app{a, _app_service, _app_reaper, _conversation_service, _conversation};
            add(t, a, id);
        }

        void app_area::add(a::generic_app* t, a::app_ptr app, const std::string& id)
        {
            REQUIRE(t);
            REQUIRE(_conversation);
            REQUIRE(t->mail());
            REQUIRE_FALSE(t->mail()->address().empty());
            
            if(auto post = _conversation->parent_post().lock())
            {
                //add to conversation
                add(t);
                s::app_metadatum meta{ t->type(), id, t->mail()->address() };
                _conversation->add_app(meta);

                //add widget mailbox to master
                post->add(t->mail());

                //send new app message to contacts in conversation
                u::bytes encoded_app;
                if(app)
                {
                    m::message app_message = *app;
                    encoded_app = u::encode(app_message);
                }

                app_pair p{app, t};
                _apps[t->mail()->address()] = p;

                ms::new_app n{t->id(), t->type(), encoded_app}; 
                _conversation->send(n);
            }
        }

        const app_map& app_area::apps() const
        {
            return _apps;
        }

        void app_area::sub_window_activated(QMdiSubWindow* w)
        {
            if(!w) w = currentSubWindow();
            if(!w) return;

            auto mw = dynamic_cast<gui::message*>(w);
            if(!mw) return;
            
            mw->clear_alert();
        }

        void app_area::clear_alerts()
        {
            for(auto sw :  subWindowList())
            {
                CHECK(sw);

                auto w  = sw->widget();
                CHECK(w);

                auto mw = dynamic_cast<gui::message*>(w);
                if(!mw) continue;

                mw->clear_alert();
            }
        }
    }
}
