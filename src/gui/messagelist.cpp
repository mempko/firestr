#include <QtGui>

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/app/chat_sample.hpp"
#include "gui/app/script_sample.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
namespace a = fire::gui::app;

namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 100;//in milliseconds
        }

        message_list::message_list(s::session_ptr session) :
            _session{session}
        {
            REQUIRE(session);

            //setup scrollbar
            _scrollbar = verticalScrollBar();
            QObject::connect(_scrollbar, SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            //setup message timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
            t->start(TIMER_SLEEP);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_scrollbar);
            INVARIANT(_session);
        }

        void message_list::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);

            //we might want to do something 
            //different with a message here
            list::add(m);
        }

        void message_list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            list::add(w);
        }

        void message_list::scroll_to_bottom(int min, int max)
        {
            Q_UNUSED(min);
            INVARIANT(_scrollbar);
            _scrollbar->setValue(max);
        }

        s::session_ptr message_list::session()
        {
            ENSURE(_session);
            return _session;
        }

        void message_list::add_new_app(const ms::new_app& n) 
        {
            INVARIANT(_session);

            if(n.type() == a::CHAT_SAMPLE)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto c = new a::chat_sample{n.id(), _session};
                    add(c);
                    post->add(c->mail());
                }
            }
            else if(n.type() == a::SCRIPT_SAMPLE)
            {
                if(auto post = _session->parent_post().lock())
                {
                    auto c = new a::script_sample{n.id(), _session};
                    add(c);
                    post->add(c->mail());
                }
            }
            else
            {
                add(new unknown_message{"unknown app type `" + n.type() + "'"});
            }
        }

        void message_list::check_mail() 
        try
        {
            INVARIANT(_session);
            INVARIANT(_session->mail());

            m::message m;
            while(_session->mail()->pop_inbox(m))
            {
                //for now show encoded message
                //TODO: use factory class to create gui from messages
                if(m.meta.type == ms::NEW_APP)
                {
                    add_new_app(m);
                }
                else
                {
                    std::stringstream s;
                    s << m;

                    add(new unknown_message{s.str()});
                }
            }
        }
        catch(std::exception& e)
        {
            std::cerr << "message_list: error in check_mail. " << e.what() << std::endl;
        }
        catch(...)
        {
            std::cerr << "message_list: unexpected error in check_mail." << std::endl;
        }
    }
}
