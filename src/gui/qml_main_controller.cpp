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

#include "gui/qml_main_controller.hpp"
#include "gui/main_win.hpp"
#include "message/master_post.hpp"
#include "message/mailbox.hpp"
#include "network/util.hpp"
#include "network/endpoint.hpp"
#include "user/user.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QVariantMap>
#include <QWidget>
#include <unordered_map>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::service;
namespace n = fire::network;
namespace sc = fire::security;

namespace fire
{
    namespace gui
    {
        struct QmlMainController::Private
        {
            main_window_context context;
            
            // Services
            m::post_office_ptr master;
            us::user_service_ptr user_service;
            conversation::conversation_service_ptr conversation_service;
            app::app_service_ptr app_service;
            app::app_reaper_ptr app_reaper;
            std::shared_ptr<sc::encrypted_channels> encrypted_channels;
            
            // UI state
            bool show_welcome = true;
            bool show_contacts = false;
            QVariantList tabs;
            QVariantList contacts;
            QVariantList conversations;
            
            // Conversation tracking
            std::vector<conversation::conversation_ptr> active_conversations;
            std::unordered_map<std::string, qml_conversation_model*> conversation_models;
        };
        
        QmlMainController::QmlMainController(const main_window_context& context, QObject* parent)
            : QObject(parent),
              d(std::make_unique<Private>())
        {
            d->context = context;
            setupServices();
            
            // Initialize UI
            createWelcomeTab();
            createContactsTab();
            updateContacts();
            
            // Show welcome screen if new user or no contacts
            d->show_welcome = d->context.user_just_created || !hasContacts();
            d->show_contacts = !d->show_welcome;
        }
        
        QmlMainController::~QmlMainController()
        {
            // Cleanup is handled by unique_ptr
        }
        
        void QmlMainController::setupServices()
        {
            REQUIRE(d);
            
            // Setup encrypted channels
            d->encrypted_channels = std::make_shared<sc::encrypted_channels>(
                d->context.user->private_key());
            
            // Create post office
            d->master = std::make_shared<m::master_post_office>(
                n::get_lan_ip(d->context.host),
                d->context.port,
                d->encrypted_channels);
            
            // Create mailbox for message handling
            auto mail = std::make_shared<m::mailbox>("qml_main");
            d->master->add(mail);
            
            // Create user service  
            us::user_service_context uc
            {
                d->context.home,
                d->context.host,
                d->context.port,
                d->context.user,
                mail,
                d->encrypted_channels,
            };
            
            d->user_service = std::make_shared<us::user_service>(uc);
            d->master->add(d->user_service->mail());
            // Note: user_service auto-starts in constructor
            
            // Create conversation service
            d->conversation_service = std::make_shared<conversation::conversation_service>(
                d->master,
                d->user_service,
                mail);
            d->master->add(d->conversation_service->mail());
            // Note: conversation_service auto-starts in constructor
            
            // Create app service - needs user_service and mailbox
            d->app_service = std::make_shared<app::app_service>(
                d->user_service,
                mail);
            
            // Create a hidden QWidget to serve as parent for app_reaper
            // This is needed because app_reaper requires a QWidget parent
            static QWidget dummy_widget;
            d->app_reaper = std::make_shared<app::app_reaper>(&dummy_widget);
        }
        
        QString QmlMainController::userName() const
        {
            REQUIRE(d);
            if (!d->user_service) return "";
            return QString::fromStdString(d->user_service->user().info().name());
        }
        
        bool QmlMainController::hasContacts() const
        {
            REQUIRE(d);
            if (!d->user_service) return false;
            return !d->user_service->user().contacts().empty();
        }
        
        bool QmlMainController::showWelcome() const
        {
            REQUIRE(d);
            return d->show_welcome;
        }
        
        bool QmlMainController::showContacts() const
        {
            REQUIRE(d);
            return d->show_contacts;
        }
        
        QVariantList QmlMainController::tabs() const
        {
            REQUIRE(d);
            return d->tabs;
        }
        
        QVariantList QmlMainController::contacts() const
        {
            REQUIRE(d);
            return d->contacts;
        }
        
        QVariantList QmlMainController::conversations() const
        {
            REQUIRE(d);
            return d->conversations;
        }
        
        QVariantList QmlMainController::installedApps() const
        {
            REQUIRE(d);
            QVariantList apps;
            
            // Add built-in apps
            QVariantMap chatApp;
            chatApp["name"] = tr("Chat");
            chatApp["appId"] = "builtin_chat";
            chatApp["icon"] = "💬";
            chatApp["builtin"] = true;
            apps.append(chatApp);
            
            QVariantMap editorApp;
            editorApp["name"] = tr("App Editor");
            editorApp["appId"] = "builtin_editor";
            editorApp["icon"] = "📝";
            editorApp["builtin"] = true;
            apps.append(editorApp);
            
            // TODO: Add user-installed apps from app_service
            if (d->app_service)
            {
                // d->app_service would have methods to get installed apps
                // For now, we just have the built-in apps
            }
            
            return apps;
        }
        
        void QmlMainController::createWelcomeTab()
        {
            REQUIRE(d);
            
            QVariantMap tab;
            tab["title"] = tr("Getting Started");
            tab["type"] = "welcome";
            tab["hasAlert"] = false;
            d->tabs.append(tab);
            
            emit tabsChanged();
        }
        
        void QmlMainController::createContactsTab()
        {
            REQUIRE(d);
            
            QVariantMap tab;
            tab["title"] = tr("People");
            tab["type"] = "contacts";
            tab["hasAlert"] = false;
            d->tabs.append(tab);
            
            emit tabsChanged();
        }
        
        void QmlMainController::updateContacts()
        {
            REQUIRE(d);
            if (!d->user_service) return;
            
            d->contacts.clear();
            
            auto& user = d->user_service->user();
            for (const auto& c : user.contacts().list())
            {
                QVariantMap contact;
                contact["id"] = QString::fromStdString(c->id());
                contact["name"] = QString::fromStdString(c->name());
                contact["online"] = d->user_service->contact_available(c->id());
                d->contacts.append(contact);
            }
            
            emit contactsChanged();
        }
        
        void QmlMainController::updateConversations()
        {
            REQUIRE(d);
            if (!d->conversation_service) return;
            
            d->conversations.clear();
            
            // Use our tracked active conversations
            for (const auto& conv : d->active_conversations)
            {
                if (!conv) continue;
                
                QVariantMap conversation;
                conversation["conversationId"] = QString::fromStdString(conv->id());
                conversation["title"] = tr("Conversation %1").arg(d->conversations.size() + 1);
                
                // Get participants
                QVariantList participants;
                for (const auto& contact : conv->contacts().list())
                {
                    if (contact)
                    {
                        QVariantMap participant;
                        participant["id"] = QString::fromStdString(contact->id());
                        participant["name"] = QString::fromStdString(contact->name());
                        participant["online"] = d->user_service->contact_available(contact->id());
                        participants.append(participant);
                    }
                }
                conversation["participants"] = participants;
                
                // TODO: Get messages from conversation
                QVariantList messages;
                conversation["messages"] = messages;
                
                // TODO: Get apps from conversation  
                QVariantList apps;
                conversation["apps"] = apps;
                
                d->conversations.append(conversation);
            }
            
            emit conversationsChanged();
        }
        
        void QmlMainController::showIdentity()
        {
            // Handled by QML dialog
            emit userNameChanged(); // Trigger update to ensure latest identity
        }
        
        void QmlMainController::addContact()
        {
            // Handled by QML dialog - dialog will call processAddContact
        }
        
        void QmlMainController::processAddContact(const QString& identity)
        {
            REQUIRE(d);
            if (!d->user_service) return;
            
            us::identity i;
            if(us::parse_identity(identity.toStdString(), i)) 
            {
                // Add greeter if not already in list
                if(!i.greeter.empty())
                {
                    bool found = false;
                    for(const auto& s : d->user_service->user().greeters())
                    {
                        auto a = s.host() + ":" + n::port_to_string(s.port());
                        if(a == i.greeter) {found = true; break;}
                    }
                    if(found) i.greeter = "";
                }
                
                // Add contact
                bool added = d->user_service->confirm_contact(i);
                if(added)
                {
                    updateContacts();
                    LOG << "Contact added successfully" << std::endl;
                }
                else
                {
                    LOG << "Failed to add contact" << std::endl;
                }
            }
            else
            {
                LOG << "Invalid identity format" << std::endl;
            }
        }
        
        QString QmlMainController::getUserIdentity(int greeterIndex)
        {
            REQUIRE(d);
            if (!d->user_service) return "";
            
            auto& user = d->user_service->user();
            auto greeters = user.greeters();
            
            std::string greeter;
            if(greeterIndex > 0 && greeterIndex <= static_cast<int>(greeters.size()))
            {
                auto& g = greeters[greeterIndex - 1]; // -1 because index 0 is "<no greeter>"
                greeter = g.host() + ":" + n::port_to_string(g.port());
            }
            
            us::identity i{ user.info(), greeter};
            auto identity = us::create_identity(i);
            return QString::fromStdString(identity);
        }
        
        QStringList QmlMainController::getGreeters()
        {
            REQUIRE(d);
            if (!d->user_service) return QStringList();
            
            QStringList result;
            result << tr("<no greeter>");
            
            for(const auto& g : d->user_service->user().greeters())
            {
                auto addr = g.host() + ":" + n::port_to_string(g.port());
                result << QString::fromStdString(addr);
            }
            
            return result;
        }
        
        void QmlMainController::emailInvite()
        {
            // TODO: Implement email invite
            LOG << "Email invite not yet implemented in QML" << std::endl;
        }
        
        void QmlMainController::startConversation()
        {
            // This is now handled by the QML dialog
            // The dialog will call createConversation with selected contact IDs
        }
        
        void QmlMainController::createConversation(const QVariantList& contactIds)
        {
            REQUIRE(d);
            if (!d->conversation_service || !d->user_service) return;
            
            conversation::conversation_ptr conv;
            
            if (contactIds.isEmpty())
            {
                // Create solo conversation
                conv = d->conversation_service->create_conversation();
                LOG << "Creating solo conversation" << std::endl;
            }
            else
            {
                // Build contact list from IDs
                us::contact_list contacts;
                for (const auto& id : contactIds)
                {
                    auto contact = d->user_service->user().contacts().by_id(id.toString().toStdString());
                    if (contact)
                    {
                        contacts.add(contact);
                    }
                }
                
                if (contacts.empty())
                {
                    // If no valid contacts found, create solo conversation
                    conv = d->conversation_service->create_conversation();
                    LOG << "No valid contacts found, creating solo conversation" << std::endl;
                }
                else
                {
                    // Create conversation with contacts
                    conv = d->conversation_service->create_conversation(contacts);
                    LOG << "Creating conversation with " << contacts.size() << " contacts" << std::endl;
                }
            }
            if (conv)
            {
                // Track the conversation
                d->active_conversations.push_back(conv);
                
                // Create conversation model
                auto model = new qml_conversation_model(
                    d->conversation_service,
                    conv,
                    d->app_service,
                    d->app_reaper,
                    this);
                d->conversation_models[conv->id()] = model;
                
                // Create conversation tab
                QVariantMap tab;
                tab["title"] = tr("Conversation %1").arg(d->active_conversations.size());
                tab["type"] = "conversation";
                tab["hasAlert"] = false;
                tab["conversationId"] = QString::fromStdString(conv->id());
                d->tabs.append(tab);
                
                // Update conversations list
                updateConversations();
                
                emit tabsChanged();
                emit conversationsChanged();
                
            }
            else
            {
                LOG << "Failed to create conversation" << std::endl;
            }
        }
        
        void QmlMainController::startConversationWith(int contactIndex)
        {
            REQUIRE(d);
            if (contactIndex < 0 || contactIndex >= d->contacts.size()) return;
            
            // TODO: Create conversation with selected contact
            LOG << "Start conversation with contact " << contactIndex << " not yet implemented" << std::endl;
        }
        
        void QmlMainController::createAppEditor()
        {
            // TODO: Implement app editor
            LOG << "App editor not yet implemented in QML" << std::endl;
        }
        
        void QmlMainController::installApp()
        {
            // TODO: Implement install app dialog
            LOG << "Install app not yet implemented in QML" << std::endl;
        }
        
        void QmlMainController::installExampleApps()
        {
            // TODO: Implement example apps installation
            LOG << "Install example apps not yet implemented in QML" << std::endl;
        }
        
        void QmlMainController::showAbout()
        {
            // Handled by QML dialog - AboutDialog.qml
            // The dialog is triggered directly from the QML menu action
        }
        
        void QmlMainController::addAppToConversation(int conversationIndex)
        {
            REQUIRE(d);
            if (conversationIndex < 0 || conversationIndex >= d->conversations.size()) return;
            
            // TODO: Implement add app to conversation
            LOG << "Add app to conversation not yet implemented" << std::endl;
        }
        
        void QmlMainController::addContactToConversation(int conversationIndex)
        {
            REQUIRE(d);
            if (conversationIndex < 0 || conversationIndex >= d->conversations.size()) return;
            
            // TODO: Implement add contact to conversation
            LOG << "Add contact to conversation not yet implemented" << std::endl;
        }
        
        void QmlMainController::switchToTab(int index)
        {
            REQUIRE(d);
            // TODO: Handle tab switching
            LOG << "Switch to tab " << index << std::endl;
        }
        
        void QmlMainController::closeTab(int index)
        {
            REQUIRE(d);
            if (index < 0 || index >= d->tabs.size()) return;
            
            // Don't close welcome or contacts tabs
            auto tab = d->tabs[index].toMap();
            if (tab["type"] == "welcome" || tab["type"] == "contacts") return;
            
            d->tabs.removeAt(index);
            emit tabsChanged();
        }
        
        qml_conversation_model* QmlMainController::getConversationModel(const QString& conversationId)
        {
            REQUIRE(d);
            
            auto it = d->conversation_models.find(conversationId.toStdString());
            if (it != d->conversation_models.end())
            {
                return it->second;
            }
            
            return nullptr;
        }
    }
}