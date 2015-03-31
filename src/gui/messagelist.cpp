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

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/app/chat.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"
#include "util/dbc.hpp"

#include <sstream>

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
        message_list::message_list(
                a::app_service_ptr app_service,
                a::app_reaper_ptr app_reaper,
                s::conversation_service_ptr conversation_s,
                s::conversation_ptr conversation) :
            _conversation_service{conversation_s},
            _conversation{conversation},
            _app_service{app_service},
            _app_reaper{app_reaper}
        {
            REQUIRE(app_service);
            REQUIRE(app_reaper);
            REQUIRE(conversation_s);
            REQUIRE(conversation);

            auto_scroll(true);
            QObject::connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(clear_alerts()));

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);
        }

        void message_list::add(app::generic_app* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);
            INVARIANT(_conversation);

            list::add(m);
        }

        void message_list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            list::add(w);
        }

        s::conversation_ptr message_list::conversation()
        {
            ENSURE(_conversation);
            return _conversation;
        }

        a::app_service_ptr message_list::app_service()
        {
            ENSURE(_app_service);
            return _app_service;
        }

        bool message_list::add_new_app(const ms::new_app& n) 
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
                add(new unknown_message{"unknown app type `" + n.type() + "'"});
                return false;
            }

            _conversation->add_app(m);

            CHECK(c);
            app_pair p{app, c};
            _apps[m.address] = p;

            return true;
        }

        void message_list::add_chat_app()
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            //create chat app
            auto t = new a::chat_app{_conversation_service, _conversation};
            add(t, nullptr, ""); 
        }

        void message_list::add_app_editor(const std::string& id)
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

        void message_list::add_script_app(const std::string& id)
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

        void message_list::add(a::generic_app* t, a::app_ptr app, const std::string& id)
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

        const app_map& message_list::apps() const
        {
            return _apps;
        }

        void message_list::clear_alerts()
        {
            INVARIANT(_layout);

            for(int i = 0; i < _layout->count(); i++)
            {
                auto itm = _layout->itemAt(i);
                auto w  = itm->widget();
                CHECK(w);

                auto mw = dynamic_cast<gui::message*>(w);
                if(!mw) continue;

                mw->clear_alert();
            }
        }
    }
}
