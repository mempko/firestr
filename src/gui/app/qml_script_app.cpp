/*
 * Copyright (C) 2025  Maxim Noah Khailo
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

#include "gui/app/qml_script_app.hpp"
#include "gui/app/qml_lua_frontend.hpp"
#include "gui/api/service.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "gui/qml_engine_holder.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace lua = fire::gui::lua;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string QML_SCRIPT_APP = "qml_script_app";

            qml_script_app::qml_script_app(
                    app_ptr a,
                    app_service_ptr app_service,
                    app_reaper_ptr app_reaper,
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{u::uuid()},
                _app{a},
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_service},
                _conversation{conversation},
                _mic_enabled{false},
                _can_clone{true},
                _is_running{false}
            {
                REQUIRE(a);
                REQUIRE(app_service);
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                init();
            }

            qml_script_app::qml_script_app(
                    const std::string& from_id,
                    const std::string& id,
                    app_ptr a,
                    app_service_ptr app_service,
                    app_reaper_ptr app_reaper,
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{id},
                _from_id{from_id},
                _app{a},
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_service},
                _conversation{conversation},
                _mic_enabled{false},
                _can_clone{true},
                _is_running{false}
            {
                REQUIRE(a);
                REQUIRE(app_service);
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                REQUIRE_FALSE(id.empty());
                init();
            }

            qml_script_app::~qml_script_app()
            {
                if(_is_running && _back)
                {
                    _back->reset();
                    _back->stop();
                }
            }

            void qml_script_app::init()
            {
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_app);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation_service->user_service(), _mail);

                // Set app name and title
                _app_name = QString::fromStdString(_app->name());
                setTitle(_app_name);
                _status = "Ready";
                
                // The QML frontend and API will be created lazily in start()
                _qml_frontend = nullptr;
                _api = nullptr;
                _back = nullptr;

                INVARIANT(_mail);
                INVARIANT(_sender);
            }

            void qml_script_app::start()
            {
                INVARIANT(_mail);
                INVARIANT(_app);
                
                LOG << "Starting script app: " << _app->name() << std::endl;
                
                // Create frontend and API if not already created
                if(!_qml_frontend)
                {
                    LOG << "Creating QML frontend on thread: " << QThread::currentThread() << std::endl;
                    
                    // Get QML engine from singleton holder
                    QQmlEngine* engine = QmlEngineHolder::instance();
                    
                    if(!engine)
                    {
                        LOG << "ERROR: Could not find QQmlEngine in singleton holder!" << std::endl;
                        update_status("Error");
                        append_output("Error: Could not find QML engine\n");
                        return;
                    }
                    
                    LOG << "Found QML engine: " << engine << std::endl;
                    
                    // Create QML frontend for Lua API
                    _qml_frontend = new qml_lua_frontend(engine, this);
                    
                    // Initialize the frontend (this will create QML components on main thread)
                    _qml_frontend->initialize();
                    
                    // Connect frontend signals to our signals
                    connect(_qml_frontend, &qml_lua_frontend::outputAppended, 
                            this, [this](const QString& text) { append_output(text); });
                    connect(_qml_frontend, &qml_lua_frontend::errorOccurred,
                            this, &qml_script_app::errorOccurred);
                    connect(_qml_frontend, &qml_lua_frontend::alertRequested,
                            this, &qml_script_app::alerted);
                    connect(_qml_frontend, &qml_lua_frontend::rootItemChanged,
                            this, [this]() {
                                LOG << "Lua root item changed signal received" << std::endl;
                                emit luaRootItemChanged();
                            });
                    
                    // Initialize Lua API with QML frontend
                    _api = std::make_shared<lua::lua_api>(
                        _app,
                        _sender,
                        _conversation,
                        _conversation_service,
                        _qml_frontend
                    );
                    
                    // Create backend client for message passing
                    _back = std::make_shared<lua::backend_client>(_api, _mail);
                }
                
                INVARIANT(_api);
                INVARIANT(_back);
                
                update_status("Starting...");
                clearOutput();
                
                _is_running = true;
                
                try
                {
                    LOG << "Running Lua code for app: " << _app->name() << std::endl;
                    LOG << "Code length: " << _app->code().length() << std::endl;
                    LOG << "Lua code:\n" << _app->code() << std::endl;
                    
                    // Run the app's Lua code through the backend
                    // This sends a RUN_CODE message to the backend's mailbox
                    _back->run(_app->code());
                    
                    // Start the backend service thread to process messages
                    _back->start();
                    
                    update_status("Running");
                    append_output("App started successfully\n");
                    LOG << "Script app started successfully: " << _app->name() << std::endl;
                }
                catch(const std::exception& e)
                {
                    LOG << "Error starting script app: " << e.what() << std::endl;
                    _is_running = false;
                    update_status("Error");
                    append_output(QString("Error: %1\n").arg(e.what()));
                    emit errorOccurred(e.what());
                }
            }

            void qml_script_app::contact_quit(const std::string& id)
            {
                if(_back) _back->contact_quit(id);
            }

            const std::string& qml_script_app::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& qml_script_app::type() const
            {
                ENSURE_FALSE(QML_SCRIPT_APP.empty());
                return QML_SCRIPT_APP;
            }

            m::mailbox_ptr qml_script_app::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            QString qml_script_app::appName() const
            {
                return _app_name;
            }

            QString qml_script_app::status() const
            {
                return _status;
            }

            QString qml_script_app::output() const
            {
                return _output;
            }

            bool qml_script_app::micEnabled() const
            {
                return _mic_enabled;
            }

            void qml_script_app::setMicEnabled(bool enabled)
            {
                if(_mic_enabled != enabled)
                {
                    _mic_enabled = enabled;
                    emit micEnabledChanged();
                }
            }

            bool qml_script_app::canClone() const
            {
                return _can_clone && _app;
            }
            
            QQuickItem* qml_script_app::luaRootItem() const
            {
                return _qml_frontend ? _qml_frontend->rootItem() : nullptr;
            }

            void qml_script_app::cloneApp()
            {
                if(!canClone()) return;
                
                INVARIANT(_app_service);
                INVARIANT(_app);
                
                // Clone the app
                auto cloned = _app->clone();
                cloned.name(cloned.name() + " (copy)");
                
                // Save the cloned app
                _app_service->save_app(cloned);
                
                append_output(QString("App cloned as '%1'\n").arg(QString::fromStdString(cloned.name())));
            }

            void qml_script_app::toggleMic()
            {
                setMicEnabled(!_mic_enabled);
            }

            void qml_script_app::clearOutput()
            {
                _output.clear();
                emit outputChanged();
            }

            void qml_script_app::runApp()
            {
                if(_is_running) return;
                start();
            }

            void qml_script_app::stopApp()
            {
                if(!_is_running) return;
                
                if(_back)
                {
                    // Reset the API and stop the backend service thread
                    _back->reset();  // This sends RESET_BACKEND message
                    _back->stop();   // This stops the service thread
                }
                
                _is_running = false;
                update_status("Stopped");
                append_output("App stopped\n");
            }

            void qml_script_app::update_status(const QString& status)
            {
                if(_status != status)
                {
                    _status = status;
                    emit statusChanged();
                }
            }

            void qml_script_app::append_output(const QString& text)
            {
                _output += text;
                emit outputChanged();
            }
        }
    }
}