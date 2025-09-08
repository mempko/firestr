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
            
            // TODO: app_reaper needs a QWidget parent which doesn't exist in QML
            // For now, we skip creating app_reaper as it's used for cleaning up closed apps
            // d->app_reaper = std::make_shared<app::app_reaper>(nullptr);
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
            
            // TODO: conversation_service doesn't expose conversations directly
            // Need to implement proper conversation management
            
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
            // TODO: Implement start conversation dialog
            LOG << "Start conversation not yet implemented in QML" << std::endl;
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
    }
}