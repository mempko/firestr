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


#include "gui/conversation.hpp"
#include "gui/unknown_message.hpp"
#include "gui/util.hpp"

#include "util/log.hpp"

#include <QtWidgets>

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
                a::app_service_ptr app_service) :
            _conversation_service{conversation_service},
            _conversation{conversation},
            _app_service{app_service},
            _messages{new message_list{app_service, conversation_service, conversation}}
        {
            REQUIRE(conversation_service);
            REQUIRE(conversation);
            REQUIRE(app_service);

            _layout = new QGridLayout;

            _contact_select = new contact_select_widget{conversation_service->user_service(), 
                [conversation](const us::user_info& u) -> bool
                {
                    return !conversation->contacts().has(u.id());
                }
            };

            _contact_list = new contact_list{conversation_service->user_service(), _conversation->contacts()};

            _add_contact = new QPushButton{"+"};
            _add_contact->setMaximumSize(20,20);
            _add_contact->setMinimumSize(20,20);
            _add_contact->setToolTip(tr("add person to this conversation"));
            connect(_add_contact, SIGNAL(clicked()), this, SLOT(add_contact()));

            update_contact_select();

            auto* cw = new QWidget;
            auto* cl = new QGridLayout;

            cw->setLayout(cl);
            cl->addWidget(_contact_select, 3,0);
            cl->addWidget(_add_contact, 3, 1);
            cl->addWidget(_contact_list, 0, 0, 2, 2);

            auto s = new QSplitter{Qt::Horizontal};
            s->addWidget(_messages);
            s->addWidget(cw);
            s->setStretchFactor(0, 1);
            s->setStretchFactor(1, 0);

            _layout->addWidget(s);

            setLayout(_layout);
            _layout->setContentsMargins(0,0,0,0);

            //setup mail timer
            _mail_service = new mail_service{_conversation->mail(), this};
            _mail_service->start();

            INVARIANT(_contact_list);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_messages);
            INVARIANT(_layout);
            INVARIANT(_app_service);
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
                _add_contact->setStyleSheet("border: 0px; background-color: 'green'; color: 'white';");
            else 
                _add_contact->setStyleSheet("border: 0px; background-color: 'grey'; color: 'white';");
        }

        void conversation_widget::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_messages);
            _messages->add(m);
        }

        void conversation_widget::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_messages);
            _messages->add(w);
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

        void conversation_widget::check_mail(m::message m)
        try
        {
            INVARIANT(_messages);
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation_service->user_service());
            INVARIANT(_conversation->mail());

            if(m.meta.type == ms::NEW_APP)
            {
                m::expect_remote(m);
                m::expect_symmetric(m);

                //add new app and get metadata
                auto meta = _messages->add_new_app(m);
                if(_conversation->has_app(meta.address)) return;
                if(meta.type.empty() || meta.address.empty()) return;
                if(meta.type == SCRIPT_APP && meta.id.empty()) return;

                _conversation->add_app(meta);
                _conversation_service->fire_conversation_alert(_conversation->id());
            }
            else if(m.meta.type == ms::REQ_APP)
            {
                m::expect_remote(m);
                m::expect_symmetric(m);
                got_req_app_message(m);
            }
            else if(m.meta.type == s::event::CONVERSATION_SYNCED)
            {
                m::expect_local(m);

                update_contacts();
                _conversation_service->fire_conversation_alert(_conversation->id());
            }
            else if(m.meta.type == s::event::CONTACT_REMOVED)
            {
                m::expect_local(m);

                s::event::contact_removed r;
                s::event::convert(m, r);

                if(r.conversation_id != _conversation->id()) return;

                auto c = _conversation_service->user_service()->by_id(r.contact_id);
                if(!c) return;

                update_contacts();
                _conversation_service->fire_conversation_alert(_conversation->id());
            }
            else if(m.meta.type == s::event::CONTACT_ADDED)
            {
                m::expect_local(m);

                s::event::contact_added r;
                s::event::convert(m, r);

                if(r.conversation_id != _conversation->id()) return;

                auto c = _conversation_service->user_service()->by_id(r.contact_id);
                if(!c) return;

                update_contacts();
                _conversation_service->fire_conversation_alert(_conversation->id());
            }
            else if(m.meta.type == us::event::CONTACT_CONNECTED)
            {
                m::expect_local(m);

                update_contacts();
            }
            else if(m.meta.type == us::event::CONTACT_DISCONNECTED)
            {
                m::expect_local(m);

                us::event::contact_disconnected r;
                us::event::convert(m, r);

                auto c = _conversation->contacts().by_id(r.id);
                if(!c) return;

                _conversation->contacts().remove(c);
                update_contacts();
            }
            else
            {
                std::stringstream s;
                s << m;

                _messages->add(new unknown_message{s.str()});
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
            INVARIANT(_app_service);

            //find the app in the current conversation with the address specified
            auto ad = std::find_if(_conversation->apps().begin(), _conversation->apps().end(), 
                    [&](const s::app_metadatum& app) -> bool { return app.address == m.app_address;});

            if(ad == _conversation->apps().end()) return;

            //encode app from app catalog if it is a script app
            u::bytes encoded_app;
            if(ad->type == SCRIPT_APP)
            {
                auto a = _app_service->load_app(ad->id);
                if(!a) return;
                m::message app_message = *a;
                encoded_app = u::encode(app_message);
            }

            //send the app back to the person who requested it.
            ms::new_app n{ad->address, ad->type, encoded_app}; 
            _conversation->send(m.from_id, n);
        }

        void conversation_widget::add_chat_app()
        {
            INVARIANT(_messages);
            _messages->add_chat_app();
        }

        void conversation_widget::add_app_editor(const std::string& id)
        {
            INVARIANT(_messages);
            _messages->add_app_editor(id);
        }

        void conversation_widget::add_script_app(const std::string& id)
        {
            INVARIANT(_messages);
            _messages->add_script_app(id);
        }
    }
}

