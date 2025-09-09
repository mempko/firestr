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

#ifndef FIRESTR_GUI_QML_CONVERSATION_MODEL_H
#define FIRESTR_GUI_QML_CONVERSATION_MODEL_H

#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "gui/app/qml_generic_app.hpp"
#include "gui/mail_service.hpp"
#include "conversation/conversation_service.hpp"
#include "message/message.hpp"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

#include <memory>
#include <unordered_map>

namespace fire
{
    namespace gui
    {
        class qml_conversation_model : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(QString conversationId READ conversationId NOTIFY conversationIdChanged)
            Q_PROPERTY(QString conversationName READ conversationName WRITE setConversationName NOTIFY conversationNameChanged)
            Q_PROPERTY(QVariantList participants READ participants NOTIFY participantsChanged)
            Q_PROPERTY(QVariantList availableApps READ availableApps NOTIFY availableAppsChanged)
            Q_PROPERTY(QVariantList activeApps READ activeApps NOTIFY activeAppsChanged)

            public:
                qml_conversation_model(
                        conversation::conversation_service_ptr,
                        conversation::conversation_ptr,
                        app::app_service_ptr,
                        app::app_reaper_ptr,
                        QObject* parent = nullptr);
                ~qml_conversation_model();

                // Properties
                QString conversationId() const;
                QString conversationName() const;
                void setConversationName(const QString& name);
                QVariantList participants() const;
                QVariantList availableApps() const;
                QVariantList activeApps() const;

            public slots:
                // App management
                void launchChatApp();
                void launchAppEditor(const QString& appId = QString());
                void launchScriptApp(const QString& appId);
                void closeApp(const QString& instanceId);
                
                // Contact management
                void addContact(const QString& contactId);
                void removeContact(const QString& contactId);
                
                // Message handling
                void check_mail(fire::message::message);
                
                // App installation
                void installApp(const QString& appPath);
                void uninstallApp(const QString& appId);
                
                // Refresh data
                void refreshParticipants();
                void refreshAvailableApps();

            signals:
                void conversationIdChanged();
                void conversationNameChanged();
                void participantsChanged();
                void availableAppsChanged();
                void activeAppsChanged();
                void appLaunched(const QString& instanceId, const QString& appType);
                void appClosed(const QString& instanceId);
                void errorOccurred(const QString& error);

            private:
                void init();
                void init_handlers();
                void update_participants();
                void update_available_apps();
                void add_app(app::qml_generic_app* app, const QString& appType);
                void remove_app(const QString& instanceId);
                void handle_new_app(const fire::message::message& m);
                void handle_req_app(const fire::message::message& m);
                void handle_contact_added(const fire::message::message& m);
                void handle_contact_removed(const fire::message::message& m);
                void handle_contact_connected(const fire::message::message& m);
                void handle_contact_disconnected(const fire::message::message& m);
                void notify_apps_contact_quit(const std::string& id);

            private:
                struct app_instance
                {
                    QString id;
                    QString type;
                    QString name;
                    app::qml_generic_app* widget;
                    app::app_ptr app;
                };

                conversation::conversation_service_ptr _conversation_service;
                conversation::conversation_ptr _conversation;
                app::app_service_ptr _app_service;
                app::app_reaper_ptr _app_reaper;
                mail_service* _mail_service;
                
                std::unordered_map<std::string, app_instance> _active_apps;
                QVariantList _available_apps_cache;
                QVariantList _participants_cache;
                QVariantList _active_apps_cache;
        };
    }
}

#endif