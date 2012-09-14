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

#include "gui/app/script_app.hpp"
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
            const std::string SCRIPT_APP = "SCRIPT_APP";

            namespace
            {
                const size_t TIMER_SLEEP = 100; //in milliseconds
                const size_t PADDING = 20;
            }

            script_app::script_app(app_ptr app, s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _session{session},
                _app{app},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app);

                init();

                INVARIANT(_api);
                INVARIANT(_session);
                INVARIANT(_app);
                INVARIANT_FALSE(_id.empty());
            }

            script_app::script_app(const std::string& id, app_ptr app, s::session_ptr session) :
                message{},
                _id{id},
                _session{session},
                _app{app},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app);
                REQUIRE_FALSE(id.empty());

                init();

                INVARIANT(_api);
                INVARIANT(_session);
                INVARIANT(_app);
                INVARIANT_FALSE(_id.empty());
            }

            script_app::~script_app()
            {
                INVARIANT(_session);
            }

            void script_app::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_session);
                INVARIANT(_app);

                _mail.reset(new m::mailbox{_id});
                _sender.reset(new ms::sender{_session->user_service(), _mail});
                _api.reset(new lua_script_api{_contacts, _sender, _session});

                //connect api widgets 
                layout()->addWidget(_api->canvas);
                _api->output->hide();

                //run script
                _api->execute(_app->code());

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup message timer
                auto *t = new QTimer(this);
                connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
                t->start(TIMER_SLEEP);

                INVARIANT(_session);
                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            const std::string& script_app::id()
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& script_app::type()
            {
                ENSURE_FALSE(SCRIPT_APP.empty());
                return SCRIPT_APP;
            }

            m::mailbox_ptr script_app::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void script_app::check_mail() 
            try
            {
                INVARIANT(_mail);
                INVARIANT(_session);
                INVARIANT(_api);

                m::message m;
                while(_mail->pop_inbox(m))
                {
                    if(m.meta.type == SIMPLE_MESSAGE)
                    {
                        simple_message sm{m};
                        _api->message_recieved(sm);
                    }
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "script_app: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cerr << "script_app: unexpected error in check_mail." << std::endl;
            }
        }
    }
}

