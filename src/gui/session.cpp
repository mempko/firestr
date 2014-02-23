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


#include "gui/session.hpp"
#include "gui/unknown_message.hpp"
#include "gui/util.hpp"

#include "util/log.hpp"

#include <QtWidgets>

namespace s = fire::session;
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
            const size_t TIMER_SLEEP = 200;//in milliseconds
            const size_t ADD_CONTACT_WIDTH = 10;
        }

        session_widget::session_widget(
                s::session_service_ptr session_service,
                s::session_ptr session,
                a::app_service_ptr app_service) :
            _session_service{session_service},
            _session{session},
            _app_service{app_service},
            _messages{new message_list{app_service, session_service, session}}
        {
            REQUIRE(session_service);
            REQUIRE(session);
            REQUIRE(app_service);

            _layout = new QGridLayout;

            _contact_select = new contact_select_widget{session_service->user_service(), 
                [session](const us::user_info& u) -> bool
                {
                    return !session->contacts().by_id(u.id());
                }
            };

            _add_contact = new QPushButton{"+"};
            _add_contact->setMaximumSize(20,20);
            connect(_add_contact, SIGNAL(clicked()), this, SLOT(add_contact()));

            update_contact_select();

            auto* cw = new QWidget;
            auto* cl = new QGridLayout;

            cw->setLayout(cl);
            cl->addWidget(_contact_select, 0,0);
            cl->addWidget(_add_contact, 0, 1);
            cl->addWidget(_messages, 1, 0, 1, 3);

            _layout->addWidget(cw);

            setLayout(_layout);
            _layout->setContentsMargins(0,0,0,0);

            //setup mail timer
            auto *t2 = new QTimer(this);
            connect(t2, SIGNAL(timeout()), this, SLOT(check_mail()));
            t2->start(TIMER_SLEEP);

            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_messages);
            INVARIANT(_layout);
            INVARIANT(_app_service);
        }
        
        void session_widget::update_contact_select()
        {
            INVARIANT(_contact_select);
            _contact_select->update_contacts();

            bool enabled = _contact_select->count() > 0;
            _add_contact->setEnabled(enabled);
        }

        void session_widget::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_messages);
            _messages->add(m);
        }

        void session_widget::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_messages);
            _messages->add(w);
        }

        s::session_ptr session_widget::session()
        {
            ENSURE(_session);
            return _session;
        }

        QWidget* contact_alert(us::user_info_ptr c, const std::string message)
        {
            REQUIRE(c);

            auto w = new QWidget;
            auto l = new QHBoxLayout;
            w->setLayout(l);

            std::stringstream s;
            s << "<b>" << c->name() << "</b> " << message;

            auto t = new QLabel{s.str().c_str()};
            l->addWidget(t);

            ENSURE(w);
            return w;
        }

        void session_widget::add_contact()
        {
            INVARIANT(_contact_select);
            INVARIANT(_session_service);
            INVARIANT(_session);

            auto contact = _contact_select->selected_contact();
            if(!contact) return;

            _session_service->add_contact_to_session(contact, _session);

            add(contact_alert(contact, convert(tr("added to session"))));
            update_contacts();
        }

        void session_widget::update_contacts()
        {
            _messages->update_contact_lists();
            update_contact_select();
        }

        void session_widget::name(const QString& s)
        {
            _name = s;
        }

        QString session_widget::name() const
        {
            return _name;
        }

        void session_widget::check_mail()
        try
        {
            INVARIANT(_messages);
            INVARIANT(_session);
            INVARIANT(_session_service);
            INVARIANT(_session_service->user_service());
            INVARIANT(_session->mail());

            m::message m;
            while(_session->mail()->pop_inbox(m))
            {
                //for now show encoded message
                //TODO: use factory class to create gui from messages
                if(m.meta.type == ms::NEW_APP)
                {
                    auto id = _messages->add_new_app(m);
                    _session->add_app_id(id);
                    _session_service->fire_session_alert(_session->id());
                }
                else if(m.meta.type == s::event::SESSION_SYNCED)
                {
                    update_contacts();
                    _session_service->fire_session_alert(_session->id());
                }
                else if(m.meta.type == s::event::CONTACT_REMOVED)
                {
                    s::event::contact_removed r;
                    s::event::convert(m, r);

                    if(r.session_id != _session->id()) continue;

                    auto c = _session_service->user_service()->by_id(r.contact_id);
                    if(!c) continue;

                    add(contact_alert(c, convert(tr("quit session"))));

                    _messages->remove_from_contact_lists(c);
                    update_contacts();
                    _session_service->fire_session_alert(_session->id());
                }
                else if(m.meta.type == s::event::CONTACT_ADDED)
                {
                    s::event::contact_added r;
                    s::event::convert(m, r);

                    if(r.session_id != _session->id()) continue;

                    auto c = _session_service->user_service()->by_id(r.contact_id);
                    if(!c) continue;

                    add(contact_alert(c, convert(tr("added to session"))));
                    update_contacts();
                    _session_service->fire_session_alert(_session->id());
                }
                else if(m.meta.type == us::event::CONTACT_CONNECTED)
                {
                    update_contacts();
                }
                else if(m.meta.type == us::event::CONTACT_DISCONNECTED)
                {
                    us::event::contact_disconnected r;
                    us::event::convert(m, r);

                    auto c = _session->contacts().by_id(r.id);
                    if(!c) continue;

                    update_contacts();
                }
                else
                {
                    std::stringstream s;
                    s << m;

                    _messages->add(new unknown_message{s.str()});
                }
            }
        }
        catch(std::exception& e)
        {
            LOG << "session: error in check_mail. " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "session: unexpected error in check_mail." << std::endl;
        }
    }
}

