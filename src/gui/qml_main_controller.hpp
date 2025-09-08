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

#ifndef FIRESTR_GUI_QML_MAIN_CONTROLLER_H
#define FIRESTR_GUI_QML_MAIN_CONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QString>
#include <memory>

#include "user/user_service.hpp"
#include "conversation/conversation_service.hpp"
#include "gui/app/app_service.hpp"
#include "gui/app/app_reaper.hpp"
#include "message/post_office.hpp"
#include "security/security_library.hpp"

namespace fire
{
    namespace gui
    {
        struct main_window_context;
        
        class QmlMainController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
            Q_PROPERTY(bool hasContacts READ hasContacts NOTIFY contactsChanged)
            Q_PROPERTY(bool showWelcome READ showWelcome NOTIFY screensChanged)
            Q_PROPERTY(bool showContacts READ showContacts NOTIFY screensChanged)
            Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
            Q_PROPERTY(QVariantList contacts READ contacts NOTIFY contactsChanged)
            Q_PROPERTY(QVariantList conversations READ conversations NOTIFY conversationsChanged)
            
        public:
            explicit QmlMainController(const main_window_context& context, QObject* parent = nullptr);
            ~QmlMainController();
            
            // Properties
            QString userName() const;
            bool hasContacts() const;
            bool showWelcome() const;
            bool showContacts() const;
            QVariantList tabs() const;
            QVariantList contacts() const;
            QVariantList conversations() const;
            
        public slots:
            // Menu actions
            void showIdentity();
            void addContact();
            void processAddContact(const QString& identity);
            QString getUserIdentity(int greeterIndex = 0);
            QStringList getGreeters();
            void emailInvite();
            void startConversation();
            void startConversationWith(int contactIndex);
            void createAppEditor();
            void installApp();
            void installExampleApps();
            void showAbout();
            
            // Conversation actions
            void addAppToConversation(int conversationIndex);
            void addContactToConversation(int conversationIndex);
            
            // Tab management
            void switchToTab(int index);
            void closeTab(int index);
            
        signals:
            void userNameChanged();
            void contactsChanged();
            void conversationsChanged();
            void tabsChanged();
            void screensChanged();
            
        private:
            void setupServices();
            void updateContacts();
            void updateConversations();
            void createWelcomeTab();
            void createContactsTab();
            
        private:
            struct Private;
            std::unique_ptr<Private> d;
        };
    }
}

#endif // FIRESTR_GUI_QML_MAIN_CONTROLLER_H