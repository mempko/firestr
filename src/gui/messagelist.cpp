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
namespace s = fire::conversation;
namespace a = fire::gui::app;
namespace u = fire::util;
namespace fg = fire::gui;


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
                s::conversation_service_ptr conversation_s,
                s::conversation_ptr conversation) :
            _app_service{app_service},
            _conversation_service{conversation_s},
            _conversation{conversation}
        {
            REQUIRE(app_service);
            REQUIRE(conversation_s);
            REQUIRE(conversation);

            auto_scroll(true);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_service);
        }

        void message_list::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);
            INVARIANT(_conversation);

            list::add(m);
        }

        void message_list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            list::add(w);
        }

        s::conversation_ptr message_list::conversation()
        {
            ENSURE(_conversation);
            return _conversation;
        }

        a::app_service_ptr message_list::app_service()
        {
            ENSURE(_app_service);
            return _app_service;
        }

        s::app_metadatum message_list::add_new_app(const ms::new_app& n) 
        {
            INVARIANT(_conversation_service);
            INVARIANT(_conversation);
            INVARIANT(_app_service);

            s::app_metadatum m;
            m.type = n.type();
            m.address = n.id();
            if(_conversation->has_app(m.address))
                return m;

            if(n.type() == a::CHAT)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    auto c = new a::chat_app{n.id(), _conversation_service, _conversation};
                    CHECK(c->mail());

                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::APP_EDITOR)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    auto app = _app_service->create_new_app();
                    app->launched_local(false);
                    auto c = new a::app_editor{n.from_id(), n.id(), _app_service, _conversation_service, _conversation, app};
                    CHECK(c->mail());

                    post->add(c->mail());
                    add(c);
                }
            }
            else if(n.type() == a::SCRIPT_APP)
            {
                if(auto post = _conversation->parent_post().lock())
                {
                    auto app = _app_service->create_app(u::decode<m::message>(n.data()));
                    auto c = new a::script_app{n.from_id(), n.id(), app, _app_service, _conversation_service, _conversation};
                    CHECK(c->mail());

                    m.id = app->id();
                    post->add(c->mail());
                    add(c);
                }
            }
            else
            {
                add(new unknown_message{"unknown app type `" + n.type() + "'"});
            }

            return m;
        }

        void message_list::add_chat_app()
        {
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            //create chat app
            auto t = new a::chat_app{_conversation_service, _conversation};
            add(t, nullptr, ""); 
        }

        void message_list::add_app_editor(const std::string& id)
        {
            INVARIANT(_app_service)
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            a::app_ptr app = id.empty() ? 
                _app_service->create_new_app() : 
                _app_service->load_app(id);

            CHECK(app);

            //create app editor
            auto t = new a::app_editor{_app_service, _conversation_service, _conversation, app};
            add(t, nullptr, ""); 
        }

        void message_list::add_script_app(const std::string& id)
        {
            INVARIANT(_app_service)
            INVARIANT(_conversation);
            INVARIANT(_conversation_service);

            //load app
            auto a = _app_service->load_app(id);
            if(!a) return;

            //create script app
            auto t = new a::script_app{a, _app_service, _conversation_service, _conversation};
            add(t, a, id);
        }

        void message_list::add(fg::message* t, a::app_ptr app, const std::string& id)
        {
            CHECK(t);
            CHECK(_conversation);
            if(auto post = _conversation->parent_post().lock())
            {
                //add to conversation
                add(t);
                _conversation->add_app({t->type(), id, t->mail()->address()});

                //add widget mailbox to master
                post->add(t->mail());

                //send new app message to contacts in conversation
                u::bytes encoded_app;
                if(app)
                {
                    m::message app_message = *app;
                    encoded_app = u::encode(app_message);
                }

                ms::new_app n{t->id(), t->type(), encoded_app}; 
                _conversation->send(n);
            }
        }
    }
}
