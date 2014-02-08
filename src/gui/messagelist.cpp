#include <QtWidgets>

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/app/chat.hpp"
#include "gui/app/app_editor.hpp"
#include "gui/app/script_app.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
namespace a = fire::gui::app;
namespace u = fire::util;


namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 100;//in milliseconds
            const size_t CW_WIDTH = 50;
        }

        message_list::message_list(
                a::app_service_ptr app_service,
                s::session_service_ptr session_s,
                s::session_ptr session) :
            _app_service{app_service},
            _session_service{session_s},
            _session{session}
        {
            REQUIRE(app_service);
            REQUIRE(session_s);
            REQUIRE(session);

            auto_scroll(true);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_app_service);
        }

        void message_list::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);
            INVARIANT(_session);

            auto contacts = _session->contacts();

            //add contact list along right side of message
            auto cw = new contact_list{_session->user_service(), contacts};
            cw->resize(CW_WIDTH, cw->height());

            auto s = new QSplitter{Qt::Horizontal};
            s->addWidget(m);
            s->addWidget(cw);
            s->setStretchFactor(0, 1);
            s->setStretchFactor(1, 0);

            //we might want to do something 
            //different with a message here
            list::add(s);
            _contact_lists.push_back(cw);
            _message_contacts.push_back(contacts);
        }

        void message_list::update_contact_lists()
        {
            INVARIANT_EQUAL(_contact_lists.size(), _message_contacts.size());

            for(size_t i = 0; i < _contact_lists.size(); i++)
            {
                auto cl = _contact_lists[i];
                const auto& mc = _message_contacts[i];

                auto is_in_session = [&](us::user_info& u) -> bool 
                {
                    return _session->contacts().has(u.id()) && mc.has(u.id());
                };

                CHECK(cl);
                cl->update_status(is_in_session);
            }
        }

        void message_list::remove_from_contact_lists(us::user_info_ptr c)
        {
            REQUIRE(c);
            for(auto& mc : _message_contacts) mc.remove(c);
        }

        void message_list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            list::add(w);
        }

        s::session_ptr message_list::session()
        {
            ENSURE(_session);
            return _session;
        }

        a::app_service_ptr message_list::app_service()
        {
            ENSURE(_app_service);
            return _app_service;
        }

        std::string message_list::add_new_app(const ms::new_app& n) 
        {
            INVARIANT(_session_service);
            INVARIANT(_session);
            INVARIANT(_app_service);

            if(n.type() == a::CHAT)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto c = new a::chat_app{n.id(), _session_service, _session};
                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::APP_EDITOR)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto app = _app_service->create_new_app();
                    app->launched_local(false);
                    auto c = new a::app_editor{n.from_id(), n.id(), _app_service, _session_service, _session, app};
                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::SCRIPT_APP)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto app = _app_service->create_app(u::decode<m::message>(n.data()));
                    auto c = new a::script_app{n.from_id(), n.id(), app, _app_service, _session_service, _session};
                    post->add(c->mail());
                    add(c);
                }
            }
            else
            {
                add(new unknown_message{"unknown app type `" + n.type() + "'"});
            }

            return n.id();
        }
    }
}
