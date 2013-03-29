/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#include "util/log.hpp"

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
                const size_t TIMER_SLEEP = 40; //in milliseconds
                const size_t PADDING = 20;
            }

            script_app::script_app(
                    app_ptr app, 
                    app_service_ptr as,
                    s::session_ptr session) :
                message{},
                _id{u::uuid()},
                _session{session},
                _app{app},
                _app_service{as},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app);
                REQUIRE(as);

                init();

                INVARIANT(_api);
                INVARIANT(_session);
                INVARIANT(_app);
                INVARIANT(_app_service);
                INVARIANT_FALSE(_id.empty());
            }

            script_app::script_app(
                    const std::string& id, 
                    app_ptr app, app_service_ptr as, 
                    s::session_ptr session) :
                message{},
                _id{id},
                _session{session},
                _app{app},
                _app_service{as},
                _contacts{session->contacts()}
            {
                REQUIRE(session);
                REQUIRE(app);
                REQUIRE(as);
                REQUIRE_FALSE(id.empty());

                init();

                INVARIANT(_api);
                INVARIANT(_session);
                INVARIANT(_app);
                INVARIANT(_app_service);
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

                _clone = new QPushButton("+");
                _clone->setMaximumSize(15,15);
                _clone->setStyleSheet("border: 0px; color: 'blue';");

                connect(_clone, SIGNAL(clicked()), this, SLOT(clone_app()));
                layout()->addWidget(_clone, 0,1);

                _canvas = new QWidget;
                _canvas_layout = new QGridLayout{_canvas};
                layout()->addWidget(_canvas, 0,0,2,1);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_session->user_service(), _mail);
                _api = std::make_shared<lua_script_api>(_contacts, _sender, _session, _canvas, _canvas_layout);

                //run script
                _api->run(_app->code());

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
                    if(m.meta.type == SCRIPT_MESSAGE)
                    {
                        auto id = m.meta.extra["from_id"].as_string();
                        if(!_session->user_service()->by_id(id)) 
                            continue;

                        script_message sm{m, _api.get()};
                        _api->message_recieved(sm);
                    }
                }
            }
            catch(std::exception& e)
            {
                LOG << "script_app: error in check_mail. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "script_app: unexpected error in check_mail." << std::endl;
            }

            void script_app::clone_app()
            {
                INVARIANT(_app_service);
                INVARIANT(_app);

                bool exists = _app_service->available_apps().count(_app->id());
                bool overwrite = false;
                if(exists)
                {
                    QMessageBox q(this);
                    q.setText("Update App?");
                    q.setInformativeText("App already exists in your collection, update it?");
                    auto *ub = q.addButton(tr("Update"), QMessageBox::ActionRole);
                    auto *cb = q.addButton(tr("New Version"), QMessageBox::ActionRole);
                    auto *canb = q.addButton(QMessageBox::Cancel);
                    auto ret = q.exec();
                    if(ret == QMessageBox::Cancel) return;

                    overwrite = q.clickedButton() == ub;
                } 

                if(!overwrite)
                {
                    QString curr_name = _app->name().c_str();
                    bool ok;
                    auto g = QInputDialog::getText(this, tr("Clone App"),
                            tr("App Name:"), QLineEdit::Normal, curr_name, &ok);

                    if (!ok || g.isEmpty()) return;

                    std::string name = convert(g);
                    _app->name(name);
                }

                if(!overwrite && exists) _app_service->clone_app(*_app);
                else _app_service->save_app(*_app);
            }

        }
    }
}

