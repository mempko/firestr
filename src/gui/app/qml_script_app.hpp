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

#ifndef FIRESTR_APP_QML_SCRIPT_APP_H
#define FIRESTR_APP_QML_SCRIPT_APP_H

#include "gui/app/qml_generic_app.hpp"
#include "gui/app/app.hpp"
#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "gui/lua/api.hpp"
#include "gui/lua/backend_client.hpp"
#include "conversation/conversation_service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <string>
#include <memory>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class qml_script_app : public qml_generic_app
            {
                Q_OBJECT
                Q_PROPERTY(QString appName READ appName NOTIFY appNameChanged)
                Q_PROPERTY(QString status READ status NOTIFY statusChanged)
                Q_PROPERTY(QString output READ output NOTIFY outputChanged)
                Q_PROPERTY(bool micEnabled READ micEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)
                Q_PROPERTY(bool canClone READ canClone NOTIFY canCloneChanged)
                Q_PROPERTY(QQuickItem* luaRootItem READ luaRootItem NOTIFY luaRootItemChanged)

                public:
                    qml_script_app(
                            app_ptr, 
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr,
                            QObject* parent = nullptr);
                    qml_script_app(
                            const std::string& from_id, 
                            const std::string& id, 
                            app_ptr, 
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr,
                            QObject* parent = nullptr);
                    ~qml_script_app();

                public:
                    virtual void start() override;
                    virtual void contact_quit(const std::string& id) override;
                    virtual const std::string& id() const override;
                    virtual const std::string& type() const override;
                    virtual fire::message::mailbox_ptr mail() override;

                    // QML properties
                    QString appName() const;
                    QString status() const;
                    QString output() const;
                    bool micEnabled() const;
                    void setMicEnabled(bool enabled);
                    bool canClone() const;
                    class QQuickItem* luaRootItem() const;

                public slots:
                    void cloneApp();
                    void toggleMic();
                    void clearOutput();
                    void runApp();
                    void stopApp();

                signals:
                    void appNameChanged();
                    void statusChanged();
                    void outputChanged();
                    void micEnabledChanged();
                    void canCloneChanged();
                    void errorOccurred(const QString& error);
                    void alerted();
                    void adjustSize();
                    void micAdded();
                    void luaRootItemChanged();

                private:
                    void init();
                    void update_status(const QString& status);  
                    void append_output(const QString& text);

                private:
                    std::string _id;
                    std::string _from_id;
                    app_ptr _app;
                    app_service_ptr _app_service;
                    app_reaper_ptr _app_reaper;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;
                    
                    // Lua API and frontend
                    class qml_lua_frontend* _qml_frontend;  // Forward declaration, use pointer for QObject
                    lua::lua_api_ptr _api;
                    lua::backend_client_ptr _back;
                    
                    // QML data
                    QString _app_name;
                    QString _status;
                    QString _output;
                    bool _mic_enabled;
                    bool _can_clone;
                    bool _is_running;
            };
            
            extern const std::string QML_SCRIPT_APP;
        }
    }
}

#endif