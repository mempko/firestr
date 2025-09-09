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

#include "gui/qml_conversation_model.hpp"
#include "gui/app/qml_chat.hpp"
#include "gui/app/qml_app_editor.hpp"
#include "gui/app/qml_script_app.hpp"
#include "gui/app/app.hpp"
#include "messages/new_app.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QVariantMap>

namespace m = fire::message;
namespace ms = fire::messages;
namespace s = fire::conversation;
namespace a = fire::gui::app;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        qml_conversation_model::qml_conversation_model(
                s::conversation_service_ptr conversation_service,
                s::conversation_ptr conversation,
                a::app_service_ptr app_service,
                a::app_reaper_ptr app_reaper,
                QObject* parent) :
            QObject{parent},
            _conversation_service{conversation_service},
            _conversation{conversation},
            _app_service{app_service},
            _app_reaper{app_reaper}
        {
            REQUIRE(conversation_service);
            REQUIRE(conversation);
            REQUIRE(app_service);
            REQUIRE(app_reaper);
            
            init();
        }

        qml_conversation_model::~qml_conversation_model()
        {
            // Clean up active apps
            for(auto& kv : _active_apps)
            {
                if(kv.second.widget)
                {
                    kv.second.widget->deleteLater();
                }
            }
            _active_apps.clear();
        }

        void qml_conversation_model::init()
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            INVARIANT(_app_service);
            
            // Use conversation's mailbox for receiving messages
            _mail_service = new mail_service{_conversation->mail(), this};
            connect(_mail_service, &mail_service::got_mail, this, &qml_conversation_model::check_mail);
            _mail_service->start();
            
            init_handlers();
            update_participants();
            update_available_apps();
        }

        void qml_conversation_model::init_handlers()
        {
            // Message handlers will be set up in check_mail
        }

        QString qml_conversation_model::conversationId() const
        {
            INVARIANT(_conversation);
            return QString::fromStdString(_conversation->id());
        }

        QString qml_conversation_model::conversationName() const
        {
            // TODO: Get conversation name if available
            return QString("Conversation");
        }

        void qml_conversation_model::setConversationName(const QString& name)
        {
            // TODO: Set conversation name if supported
            emit conversationNameChanged();
        }

        QVariantList qml_conversation_model::participants() const
        {
            return _participants_cache;
        }

        QVariantList qml_conversation_model::availableApps() const
        {
            return _available_apps_cache;
        }

        QVariantList qml_conversation_model::activeApps() const
        {
            return _active_apps_cache;
        }

        void qml_conversation_model::update_participants()
        {
            INVARIANT(_conversation);
            
            _participants_cache.clear();
            
            // Add self
            QVariantMap self;
            self["id"] = QString::fromStdString(_conversation_service->user_service()->user().info().id());
            self["name"] = QString::fromStdString(_conversation_service->user_service()->user().info().name());
            self["online"] = true;
            self["isSelf"] = true;
            _participants_cache.append(self);
            
            // Add contacts
            for(const auto& c : _conversation->contacts().list())
            {
                if(!c) continue;
                
                QVariantMap participant;
                participant["id"] = QString::fromStdString(c->id());
                participant["name"] = QString::fromStdString(c->name());
                participant["online"] = _conversation_service->user_service()->contact_available(c->id());
                participant["isSelf"] = false;
                _participants_cache.append(participant);
            }
            
            emit participantsChanged();
        }

        void qml_conversation_model::update_available_apps()
        {
            INVARIANT(_app_service);
            
            _available_apps_cache.clear();
            
            // Add built-in apps
            QVariantMap chat;
            chat["id"] = "builtin_chat";
            chat["name"] = "Chat";
            chat["icon"] = "💬";
            chat["type"] = "builtin";
            _available_apps_cache.append(chat);
            LOG << "Added chat app to available apps" << std::endl;
            
            QVariantMap editor;
            editor["id"] = "builtin_editor";
            editor["name"] = "App Editor";
            editor["icon"] = "📝";
            editor["type"] = "builtin";
            _available_apps_cache.append(editor);
            LOG << "Added app editor to available apps" << std::endl;
            
            // Add installed apps
            auto installed_apps = _app_service->available_apps();
            LOG << "Found " << installed_apps.size() << " installed apps" << std::endl;
            for(const auto& kv : installed_apps)
            {
                QVariantMap app;
                app["id"] = QString::fromStdString(kv.second.id);
                app["name"] = QString::fromStdString(kv.second.name);
                app["icon"] = "📦";
                app["type"] = "script";
                _available_apps_cache.append(app);
                LOG << "Added installed app: " << kv.second.name << " (id: " << kv.second.id << ")" << std::endl;
            }
            
            LOG << "Total available apps: " << _available_apps_cache.size() << std::endl;
            emit availableAppsChanged();
        }

        void qml_conversation_model::launchChatApp()
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            
            LOG << "Launching chat app..." << std::endl;
            
            // Check if chat is already open
            for(const auto& kv : _active_apps)
            {
                if(kv.second.type == "chat")
                {
                    LOG << "Chat already open, focusing..." << std::endl;
                    // TODO: Focus existing chat
                    return;
                }
            }
            
            // Create chat app
            LOG << "Creating new chat app..." << std::endl;
            auto chat = new a::qml_chat_app{_conversation_service, _conversation, this};
            add_app(chat, "chat");
            chat->start();
            LOG << "Chat app started" << std::endl;
        }

        void qml_conversation_model::launchAppEditor(const QString& appId)
        {
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            
            // Load or create app
            a::app_ptr app = nullptr;
            if(!appId.isEmpty())
            {
                app = _app_service->load_app(appId.toStdString());
            }
            
            // Create app editor
            auto editor = new a::qml_app_editor{
                _app_service, 
                _app_reaper, 
                _conversation_service, 
                _conversation, 
                app,
                this
            };
            
            add_app(editor, "editor");
            editor->start();
        }

        void qml_conversation_model::launchScriptApp(const QString& appId)
        {
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            
            if(appId.isEmpty()) 
            {
                emit errorOccurred("No app ID specified");
                return;
            }
            
            // Load app
            auto app = _app_service->load_app(appId.toStdString());
            if(!app) 
            {
                emit errorOccurred("Failed to load app: " + appId);
                return;
            }
            
            // Create script app
            auto script = new a::qml_script_app{
                app,
                _app_service,
                _app_reaper,
                _conversation_service,
                _conversation,
                this
            };
            
            // Set the app name from the loaded app
            script->setTitle(QString::fromStdString(app->name()));
            
            add_app(script, "script");
            // Don't start immediately - wait for QML side to call runApp()
        }

        void qml_conversation_model::add_app(a::qml_generic_app* app, const QString& appType)
        {
            REQUIRE(app);
            
            LOG << "Adding app of type: " << appType.toStdString() << std::endl;
            
            // Generate unique instance ID
            auto instance_id = u::uuid();
            
            // Create app instance
            app_instance instance;
            instance.id = QString::fromStdString(instance_id);
            instance.type = appType;
            instance.name = app->title();
            instance.widget = app;
            
            LOG << "App instance created - id: " << instance_id << ", name: " << instance.name.toStdString() << std::endl;
            
            // Store in map
            _active_apps[instance_id] = instance;
            
            // Update active apps cache
            QVariantMap app_data;
            app_data["instanceId"] = instance.id;
            app_data["type"] = instance.type;
            app_data["name"] = instance.name;
            app_data["x"] = 50 + _active_apps_cache.size() * 30;
            app_data["y"] = 50 + _active_apps_cache.size() * 30;
            app_data["width"] = 400;
            app_data["height"] = 300;
            app_data["appModel"] = QVariant::fromValue(app);  // Pass the app model
            _active_apps_cache.append(app_data);
            
            LOG << "Active apps cache updated, now has " << _active_apps_cache.size() << " apps" << std::endl;
            
            emit activeAppsChanged();
            emit appLaunched(instance.id, instance.type);
        }

        void qml_conversation_model::closeApp(const QString& instanceId)
        {
            auto it = _active_apps.find(instanceId.toStdString());
            if(it == _active_apps.end()) return;
            
            // Delete widget
            if(it->second.widget)
            {
                it->second.widget->deleteLater();
            }
            
            // Remove from map
            _active_apps.erase(it);
            
            // Update cache
            for(int i = 0; i < _active_apps_cache.size(); ++i)
            {
                auto map = _active_apps_cache[i].toMap();
                if(map["instanceId"].toString() == instanceId)
                {
                    _active_apps_cache.removeAt(i);
                    break;
                }
            }
            
            emit activeAppsChanged();
            emit appClosed(instanceId);
        }

        void qml_conversation_model::addContact(const QString& contactId)
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            
            // TODO: Implement contact addition
            update_participants();
        }

        void qml_conversation_model::removeContact(const QString& contactId)
        {
            INVARIANT(_conversation);
            
            // TODO: Implement contact removal
            update_participants();
        }

        void qml_conversation_model::check_mail(m::message msg)
        {
            if(msg.meta.type == ms::NEW_APP)
            {
                handle_new_app(msg);
            }
            else if(msg.meta.type == ms::REQ_APP)
            {
                handle_req_app(msg);
            }
            else if(msg.meta.type == "contact_added")
            {
                handle_contact_added(msg);
            }
            else if(msg.meta.type == "contact_removed")
            {
                handle_contact_removed(msg);
            }
            else if(msg.meta.type == "contact_connected")
            {
                handle_contact_connected(msg);
            }
            else if(msg.meta.type == "contact_disconnected")
            {
                handle_contact_disconnected(msg);
            }
        }

        void qml_conversation_model::handle_new_app(const m::message& m)
        {
            // Handle new app received from contact
            ms::new_app n{m};
            
            // Install app if not already installed
            auto app_meta = _app_service->available_apps();
            bool exists = app_meta.find(n.id()) != app_meta.end();
            if(!exists)
            {
                // TODO: Handle app installation from message
                update_available_apps();
            }
        }

        void qml_conversation_model::handle_req_app(const m::message& m)
        {
            // Handle app request from contact
            // TODO: Implement app sharing
        }

        void qml_conversation_model::handle_contact_added(const m::message& m)
        {
            update_participants();
        }

        void qml_conversation_model::handle_contact_removed(const m::message& m)
        {
            auto id = m.meta.extra["id"].as_string();
            notify_apps_contact_quit(id);
            update_participants();
        }

        void qml_conversation_model::handle_contact_connected(const m::message& m)
        {
            update_participants();
        }

        void qml_conversation_model::handle_contact_disconnected(const m::message& m)
        {
            auto id = m.meta.extra["id"].as_string();
            notify_apps_contact_quit(id);
            update_participants();
        }

        void qml_conversation_model::notify_apps_contact_quit(const std::string& id)
        {
            for(auto& kv : _active_apps)
            {
                if(kv.second.widget)
                {
                    kv.second.widget->contact_quit(id);
                }
            }
        }

        void qml_conversation_model::installApp(const QString& appPath)
        {
            // TODO: Implement app installation from file
            update_available_apps();
        }

        void qml_conversation_model::uninstallApp(const QString& appId)
        {
            INVARIANT(_app_service);
            
            // TODO: Implement app uninstallation
            update_available_apps();
        }

        void qml_conversation_model::refreshParticipants()
        {
            update_participants();
        }

        void qml_conversation_model::refreshAvailableApps()
        {
            update_available_apps();
        }
    }
}