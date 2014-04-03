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
 */

#include <QtWidgets>

#include "gui/app/script_app.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>
#include <QThread>

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
                _app_service{as},
                _contacts{conversation->contacts()}
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
                _app_service{as},
                _contacts{conversation->contacts()}
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
                INVARIANT(_mail_service);

                _mail_service->done();
                LOG << "closed app " << _app->name() << "(" << _app->id() << ")" << std::endl;
            }

            void script_app::init()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation);
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
                _sender = std::make_shared<ms::sender>(_conversation->user_service(), _mail);
                _api = std::make_shared<l::lua_api>(_app, _contacts, _sender, _conversation, _conversation_service, _canvas, _canvas_layout);
                _api->who_started_id = _from_id;

                //run script
                _api->run(_app->code());

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup mail service
                _mail_service = new mail_service{_mail, this};
                qRegisterMetaType<fire::message::message>("fire::message::message");
                connect(_mail_service, SIGNAL(got_mail(fire::message::message)), this, SLOT(check_mail(fire::message::message)));
                _mail_service->start();

                INVARIANT(_conversation);
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

            void script_app::check_mail(m::message m) 
            try
            {
                INVARIANT(_conversation);
                INVARIANT(_api);

                if(m::is_remote(m)) m::expect_symmetric(m);
                else m::expect_plaintext(m);

                if(m.meta.type == l::SCRIPT_MESSAGE)
                {
                    bool local = m.meta.extra.has("local_app_id");
                    auto id = m.meta.extra["from_id"].as_string();

                    if(!local && !_conversation->user_service()->by_id(id)) 
                        return;

                    l::script_message sm{m, _api.get()};
                    _api->message_received(sm);
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

