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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * Botan library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than Botan. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */


#include "gui/conversation.hpp"
#include "gui/unknown_message.hpp"
#include "gui/util.hpp"

#include "util/log.hpp"

#include <QtWidgets>

#include <functional>

namespace u = fire::util;
namespace s = fire::conversation;
namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace a = fire::gui::app;

namespace fire
{
    namespace gui
    {
        namespace 
        {
            const size_t ADD_CONTACT_WIDTH = 10;
            const std::string SCRIPT_APP = "SCRIPT_APP";
        }

        conversation_widget::conversation_widget(
                s::conversation_service_ptr conversation_service,
                s::conversation_ptr conversation,
                a::app_service_ptr app_service,
                a::app_reaper_ptr app_reaper) :
            _app_area{new app_area{app_service, app_reaper, conversation_service, conversation}},
            _conversation{conversation},
            _conversation_service{conversation_service},
            _app_service{app_service},
            _app_reaper{app_reaper}
        {
            REQUIRE(conversation_service);
            REQUIRE(conversation);
            REQUIRE(app_service);
            REQUIRE(app_reaper);

            init_handlers();

            _layout = new QGridLayout;

            _contact_select = new contact_select_widget{conversation_service->user_service(), 
                [conversation](const us::user_info& u) -> bool
                {
                    return !conversation->contacts().has(u.id());
                }
            };

            _contact_list = new contact_list{conversation_service->user_service(), _conversation->contacts()};
            _add_contact = new QPushButton;
            make_add_contact_small(*_add_contact);
            _add_contact->setToolTip(tr("Add person to this conversation"));
            connect(_add_contact, SIGNAL(clicked()), this, SLOT(add_contact()));

            update_contact_select();

            auto* cw = new QWidget;
            auto* cl = new QGridLayout;

            cw->setLayout(cl);
            cw->setContentsMargins(0,0,0,0);

            cl->addWidget(_contact_select, 3,0);
            cl->addWidget(_add_contact, 3, 1);
            cl->addWidget(_contact_list, 0, 0, 2, 2);
            cl->setContentsMargins(0,0,0,0);

            auto s = new QSplitter{Qt::Horizontal};
            s->setFrameShape(QFrame::NoFrame);
            s->setContentsMargins(0,0,0,0);
            s->addWidget(_app_area);
            s->addWidget(cw);
            s->setStretchFactor(0, 1);
            s->setStretchFactor(1, 0);

            _layout->addWidget(s);
            _layout->setContentsMargins(0,0,0,0);

            setLayout(_layout);

            //setup mail timer
            _mail_service = new mail_service{_conversation->mail(), this};
            _mail_service->start();

            INVARIANT(_contact_list);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_area);
            INVARIANT(_layout);
            INVARIANT(_app_service);
            INVARIANT(_app_reaper);
            INVARIANT(_mail_service);
        }

        conversation_widget::~conversation_widget()
        {
            INVARIANT(_mail_service);
            _mail_service->done();
        }
        
        void conversation_widget::update_contact_select()
        {
            INVARIANT(_contact_select);
            _contact_select->update_contacts();

            bool enabled = _contact_select->count() > 0;
            _add_contact->setEnabled(enabled);
            if(enabled) 
                _add_contact->setStyleSheet("border: 0px; color: 'green';");
            else 
                _add_contact->setStyleSheet("border: 0px; color: 'grey';");
        }

        void conversation_widget::add(app::generic_app* m)
        {
            REQUIRE(m);
            INVARIANT(_app_area);
            _app_area->add(m);
        }

        s::conversation_ptr conversation_widget::conversation()
        {
            ENSURE(_conversation);
            return _conversation;
        }

        void conversation_widget::add_contact()
        {
            INVARIANT(_contact_select);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);

            auto contact = _contact_select->selected_contact();
            if(!contact) return;

            _conversation_service->add_contact_to_conversation(contact, _conversation);
            update_contacts();
        }

        void conversation_widget::update_contacts()
        {
            INVARIANT(_contact_list);
            INVARIANT(_conversation);

            _contact_list->update(_conversation->contacts());
            _contact_list->update_status(true);
            update_contact_select();
        }

        void conversation_widget::name(const QString& s)
        {
            _name = s;
        }

        QString conversation_widget::name() const
        {
            return _name;
        }

        void conversation_widget::init_handlers()
        {
            using std::bind;
            using namespace std::placeholders;

            _sm.handle(ms::NEW_APP, 
                    bind(&conversation_widget::received_new_app, this, _1));
            _sm.handle(ms::REQ_APP, 
                    bind(&conversation_widget::received_req_app, this, _1));
            _sm.handle(s::event::CONVERSATION_SYNCED, 
                    bind(&conversation_widget::received_conversation_synced, this, _1));
            _sm.handle(s::event::CONTACT_REMOVED, 
                    bind(&conversation_widget::received_contact_removed, this, _1));
            _sm.handle(s::event::CONTACT_ADDED, 
                    bind(&conversation_widget::received_contact_added, this, _1));
            _sm.handle(us::event::CONTACT_CONNECTED, 
                    bind(&conversation_widget::received_contact_connected, this, _1));
            _sm.handle(us::event::CONTACT_DISCONNECTED, 
                    bind(&conversation_widget::received_contact_disconnected, this, _1));
            _sm.handle(us::event::CONTACT_ACTIVITY_CHANGED, 
                    bind(&conversation_widget::received_contact_activity_changed, this, _1));
        }

        void conversation_widget::received_new_app(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, ms::NEW_APP);

            INVARIANT(_app_area);
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            m::expect_remote(m);
            m::expect_symmetric(m);

            //add new app and get metadata
            if(!_app_area->add_new_app(m)) return;
            _conversation_service->fire_conversation_alert(_conversation->id(), false);
        }

        void conversation_widget::received_req_app(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, ms::REQ_APP);

            m::expect_remote(m);
            m::expect_symmetric(m);
            got_req_app_message(m);
        }

        void conversation_widget::received_conversation_synced(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::CONVERSATION_SYNCED);

            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            m::expect_local(m);

            update_contacts();
            _conversation_service->fire_conversation_alert(_conversation->id(), false);
        }

        void conversation_widget::received_contact_removed(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::CONTACT_REMOVED);

            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation_service->user_service());

            m::expect_local(m);

            s::event::contact_removed r;
            r.from_message(m);

            if(r.conversation_id != _conversation->id()) return;

            auto c = _conversation_service->user_service()->by_id(r.contact_id);
            if(!c) return;

            update_contacts();
            notify_apps_contact_quit(r.contact_id);
            _conversation_service->fire_conversation_alert(_conversation->id(), false);
        }

        void conversation_widget::received_contact_added(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, s::event::CONTACT_ADDED);

            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation_service->user_service());

            m::expect_local(m);

            s::event::contact_added r;
            r.from_message(m);

            if(r.conversation_id != _conversation->id()) return;

            auto c = _conversation_service->user_service()->by_id(r.contact_id);
            if(!c) return;

            update_contacts();
            _conversation_service->fire_conversation_alert(_conversation->id(), false);
        }

        void conversation_widget::received_contact_connected(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_CONNECTED);

            m::expect_local(m);
            update_contacts();
        }

        void conversation_widget::received_contact_disconnected(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_DISCONNECTED);
            INVARIANT(_conversation);
            INVARIANT(_conversation->mail());
            
            m::expect_local(m);

            us::event::contact_disconnected r;
            r.from_message(m);

            auto c = _conversation->contacts().by_id(r.id);
            if(!c) return;

            _conversation->remove_contact(r.id);
            update_contacts();
            notify_apps_contact_quit(r.id);
        }

        void conversation_widget::received_contact_activity_changed(const m::message& m)
        {
            REQUIRE_EQUAL(m.meta.type, us::event::CONTACT_ACTIVITY_CHANGED);
            INVARIANT(_conversation);
            INVARIANT(_conversation->mail());
            
            m::expect_local(m);

            us::event::contact_activity_changed r;
            r.from_message(m);

            auto c = _conversation->contacts().by_id(r.id);
            if(!c) return;

            update_contacts();
        }

        void conversation_widget::notify_apps_contact_quit(const std::string& id)
        {
            INVARIANT(_app_area);
            for(auto p : _app_area->apps()) 
            {
                CHECK(p.second.widget);
                p.second.widget->contact_quit(id);
            }
        }

        void conversation_widget::check_mail(m::message m)
        try
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation->mail());

            if(!_sm.handle(m))
            {
                LOG << "conversation: unkown message `" << m.meta.type << "' in " << _conversation->mail()->address() << std::endl;
            }
        }
        catch(std::exception& e)
        {
            LOG << "conversation: error in check_mail. " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "conversation: unexpected error in check_mail." << std::endl;
        }

        void conversation_widget::got_req_app_message(const messages::request_app& m)
        {
            INVARIANT(_conversation);
            INVARIANT(_app_area);

            //find the app in the current conversation with the address specified
            auto a = _conversation->apps().find(m.app_address);

            if(a == _conversation->apps().end()) return;

            const auto& ad = a->second;

            //encode app from app catalog if it is a script app
            u::bytes encoded_app;
            if(ad.type == SCRIPT_APP)
            {
                auto ap = _app_area->apps().find(ad.address);
                if(ap == _app_area->apps().end()) return;

                CHECK(ap->second.app);
                m::message app_message = *ap->second.app;
                encoded_app = u::encode(app_message);
            }

            //send the app back to the person who requested it.
            ms::new_app n{ad.address, ad.type, encoded_app}; 
            _conversation->send(m.from_id, n);
        }

        void conversation_widget::add_chat_app()
        {
            INVARIANT(_app_area);
            _app_area->add_chat_app();
        }

        void conversation_widget::add_app_editor(const std::string& id)
        {
            INVARIANT(_app_area);
            _app_area->add_app_editor(id);
        }

        void conversation_widget::add_script_app(const std::string& id)
        {
            INVARIANT(_app_area);
            _app_area->add_script_app(id);
        }

        void conversation_widget::clear_alerts()
        {
            INVARIANT(_app_area);
            _app_area->clear_alerts();
        }
    }
}

