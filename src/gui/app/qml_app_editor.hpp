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

#ifndef FIRESTR_APP_QML_APP_EDITOR_H
#define FIRESTR_APP_QML_APP_EDITOR_H

#include "gui/app/qml_generic_app.hpp"
#include "gui/app/app.hpp"
#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "gui/lua/api.hpp"
#include "conversation/conversation_service.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QColor>

#include <string>
#include <memory>
#include <unordered_map>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class qml_app_editor : public qml_generic_app
            {
                Q_OBJECT
                Q_PROPERTY(QString code READ code WRITE setCode NOTIFY codeChanged)
                Q_PROPERTY(QString output READ output NOTIFY outputChanged)
                Q_PROPERTY(QString status READ status NOTIFY statusChanged)
                Q_PROPERTY(QString appName READ appName WRITE setAppName NOTIFY appNameChanged)
                Q_PROPERTY(QVariantList dataItems READ dataItems NOTIFY dataItemsChanged)
                Q_PROPERTY(bool canSave READ canSave NOTIFY canSaveChanged)
                Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
                Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
                Q_PROPERTY(QVariantList collaborators READ collaborators NOTIFY collaboratorsChanged)

                public:
                    qml_app_editor(
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr, 
                            app_ptr a = nullptr,
                            QObject* parent = nullptr);
                    qml_app_editor(
                            const std::string& from_id, 
                            const std::string& id, 
                            app_service_ptr, 
                            app_reaper_ptr, 
                            conversation::conversation_service_ptr, 
                            conversation::conversation_ptr, 
                            app_ptr a = nullptr,
                            QObject* parent = nullptr);
                    ~qml_app_editor();

                public:
                    virtual void start() override;
                    virtual void contact_quit(const std::string& id) override;
                    virtual const std::string& id() const override;
                    virtual const std::string& type() const override;
                    virtual fire::message::mailbox_ptr mail() override;
                    
                    // QML properties
                    QString code() const;
                    void setCode(const QString& code);
                    QString output() const;
                    QString status() const;
                    QString appName() const;
                    void setAppName(const QString& name);
                    QVariantList dataItems() const;
                    bool canSave() const;
                    bool isRunning() const;
                    int cursorPosition() const;
                    void setCursorPosition(int pos);
                    QVariantList collaborators() const;

                public slots:
                    void runScript();
                    void stopScript();
                    void saveApp();
                    void exportApp();
                    void clearOutput();
                    void sendScript();
                    void addDataItem();
                    void removeDataItem(const QString& key);
                    void updateDataItem(const QString& key, const QVariant& value);
                    void loadDataFromFile(const QString& key);

                signals:
                    void codeChanged();
                    void outputChanged();
                    void statusChanged();
                    void appNameChanged();
                    void dataItemsChanged();
                    void canSaveChanged();
                    void isRunningChanged();
                    void cursorPositionChanged();
                    void collaboratorsChanged();
                    void outputAppended(const QString& text);
                    void errorOccurred(const QString& error);
                    void scriptStarted();
                    void scriptStopped();

                private:
                    void init();
                    void init_handlers();
                    void init_data();
                    void update_app_code();
                    void update_status(const QString& status);
                    void append_output(const QString& text);
                    void send_all(const fire::message::message& m);
                    void send_cursor_position();
                    void handle_remote_code(const fire::message::message& m);
                    void handle_remote_cursor(const fire::message::message& m);
                    void handle_remote_data(const fire::message::message& m);
                    void update_collaborator_cursor(const std::string& id, int pos);
                    
                    // QML Frontend implementation
                    class qml_frontend;

                private:
                    std::string _id;
                    std::string _from_id;
                    app_service_ptr _app_service;
                    app_reaper_ptr _app_reaper;
                    conversation::conversation_service_ptr _conversation_service;
                    conversation::conversation_ptr _conversation;
                    fire::message::mailbox_ptr _mail;
                    messages::sender_ptr _sender;
                    app_ptr _app;
                    
                    // QML data
                    QString _code;
                    QString _output;
                    QString _status;
                    QString _app_name;
                    QVariantList _data_items;
                    bool _can_save;
                    bool _is_running;
                    int _cursor_position;
                    
                    // Collaborator cursors
                    struct collaborator_info
                    {
                        std::string name;
                        int cursor_pos;
                        QColor color;
                    };
                    std::unordered_map<std::string, collaborator_info> _collaborators;
                    
                    // Lua API and frontend
                    std::unique_ptr<qml_frontend> _frontend;
                    lua::lua_api_ptr _api;
                    
                    // Update timer
                    bool _code_changed;
                    bool _updating;
            };
            
            extern const std::string QML_APP_EDITOR;
        }
    }
}

#endif