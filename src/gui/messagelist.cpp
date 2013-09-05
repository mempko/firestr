#include <QtWidgets>

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/app/chat_sample.hpp"
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
                s::session_ptr session) :
            _app_service{app_service},
            _session{session}
        {
            REQUIRE(app_service);
            REQUIRE(session);

            auto_scroll(true);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_session);
            INVARIANT(_app_service);
        }

        void message_list::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);
            INVARIANT(_session);

            //add contact list along right side of message
            auto cw = new contact_list{_session->user_service(), _session->contacts()};
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
        }

        void message_list::update_contact_lists()
        {
            for(auto cl : _contact_lists) 
            {
                CHECK(cl);
                cl->update_status();
            }
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
            INVARIANT(_session);
            INVARIANT(_app_service);

            if(n.type() == a::CHAT_SAMPLE)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto c = new a::chat_sample{n.id(), _session};
                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::SCRIPT_SAMPLE)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto c = new a::app_editor{n.id(), _app_service, _session, nullptr};
                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::SCRIPT_APP)
            {
                if(auto post = _session->parent_post().lock())
                {
                    app::app_ptr app{new app::app{u::decode<m::message>(n.data())}};

                    auto c = new a::script_app{n.id(), app, _app_service, _session};
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
