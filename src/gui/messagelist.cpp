#include <QtGui>

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/test_message.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;

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

            list::add(m);
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

        void message_list::check_mail() 
        {
            INVARIANT(_session);
            INVARIANT(_session->mail());

            m::message m;
            while(_session->mail()->pop_inbox(m))
            {
                //for now show encoded message
                //TODO: use factory class to create gui from messages
                if(m.meta.type == ms::TEST_MESSAGE)
                {
                    add(new test_message{m, _session->sender()});
                }
                else
                {
                    std::stringstream s;
                    s << m;

                    add(new unknown_message{s.str()});
                }
            }
        }
    }
}
