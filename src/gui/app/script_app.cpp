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

#include "gui/app/script_app.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace l = fire::gui::lua;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string SCRIPT_APP = "SCRIPT_APP";

            namespace
            {
                const size_t TIMER_SLEEP = 40; //in milliseconds
                const size_t PADDING = 20;
            }

            script_app::script_app(
                    app_ptr app, 
                    app_service_ptr as,
                    s::conversation_service_ptr conversation_s,
                    s::conversation_ptr conversation) :
                message{},
                _from_id{conversation->user_service()->user().info().id()},
                _id{u::uuid()},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _app{app},
                _app_service{as}
            {
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);
                REQUIRE(as);

                init();

                INVARIANT(_api);
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app);
                INVARIANT(_app_service);
                INVARIANT_FALSE(_id.empty());
            }

            script_app::script_app(
                    const std::string& from_id, 
                    const std::string& id, 
                    app_ptr app, app_service_ptr as, 
                    s::conversation_service_ptr conversation_s,
                    s::conversation_ptr conversation) :
                message{},
                _from_id{from_id},
                _id{id},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _app{app},
                _app_service{as}
            {
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);
                REQUIRE(as);
                REQUIRE_FALSE(id.empty());

                init();

                INVARIANT(_api);
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app);
                INVARIANT(_app_service);
                INVARIANT_FALSE(_id.empty());
            }

            script_app::~script_app()
            {
                INVARIANT(_app);
                INVARIANT(_conversation);

                LOG << "closed app " << _app->name() << "(" << _app->id() << ")" << std::endl;
                
            }

            void script_app::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation);
                INVARIANT(_app);

                //setup frontend
                
                _clone = new QPushButton("+");
                _clone->setMaximumSize(15,15);
                _clone->setMinimumSize(15,15);
                _clone->setStyleSheet("border: 0px; background-color: 'light green'; color: 'white';");
                _clone->setToolTip(tr("add app to your collection"));

                connect(_clone, SIGNAL(clicked()), this, SLOT(clone_app()));
                layout()->addWidget(_clone, 0,1);

                _canvas = new QWidget;
                _canvas_layout = new QGridLayout{_canvas};
                layout()->addWidget(_canvas, 0,0,2,1);
                auto front = std::make_shared<qtw::qt_frontend>(_canvas, _canvas_layout, nullptr);
                _front = std::make_shared<qtw::qt_frontend_client>(front);

                //setup mail
                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation->user_service(), _mail);

                //setup api and backend
                _api = std::make_shared<l::lua_api>(
                        _app, 
                        _sender, 
                        _conversation, 
                        _conversation_service, 
                        _front.get());
                _api->who_started_id = _from_id;

                _back = std::make_shared<l::backend_client>(_api.get(), _mail); 

                //assign backend to frontend
                _front->set_backend(_back.get());

                //run script and start backend on seperate thread
                _back->run(_app->code());
                _back->start();

                //setup mail service
                INVARIANT(_api);
                INVARIANT(_front);
                INVARIANT(_back);
                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            const std::string& script_app::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& script_app::type() const
            {
                ENSURE_FALSE(SCRIPT_APP.empty());
                return SCRIPT_APP;
            }

            m::mailbox_ptr script_app::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void script_app::received_script_message(const m::message& m)
            {
                REQUIRE_EQUAL(m.meta.type, l::SCRIPT_MESSAGE);

                INVARIANT(_conversation);
                INVARIANT(_api);

                bool local = m.meta.extra.has("local_app_id");
                auto id = m.meta.extra["from_id"].as_string();

                if(!local && !_conversation->user_service()->by_id(id)) 
                    return;

                l::script_message sm{m, _api.get()};
                _api->message_received(sm);
            }

            void script_app::received_event_message(const m::message& m)
            {
                REQUIRE_EQUAL(m.meta.type, l::EVENT_MESSAGE);
                INVARIANT(_conversation);
                INVARIANT(_api);

                auto id = m.meta.extra["from_id"].as_string();

                if(!_conversation->user_service()->by_id(id)) 
                    return;

                l::event_message em{m, _api.get()};
                _api->event_received(em);
            }

            void script_app::clone_app()
            {
                INVARIANT(_app_service);
                INVARIANT(_app);

                install_app_gui(*_app, *_app_service, this);
            }

        }
    }
}

