#include <QtGui>

#include "gui/messagelist.hpp"
#include "gui/textmessage.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace m = fire::message;

namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 100;//in milliseconds
        }

        message_list::message_list(const std::string& name) :
            _name{name},
            _mail{new m::mailbox{name}}
        {
            //setup scrollbar
            _scrollbar = verticalScrollBar();
            QObject::connect(_scrollbar, SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            //setup message timer
            QTimer *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
            t->start(TIMER_SLEEP);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_scrollbar);
            INVARIANT(_mail);
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

        const m::mailbox_ptr message_list::mail() const
        {
            ENSURE(_mail);
            return _mail;
        }

        m::mailbox_ptr message_list::mail()
        {
            ENSURE(_mail);
            return _mail;
        }

        void message_list::check_mail() 
        {
            INVARIANT(_mail);

            m::message m;
            while(_mail->pop_inbox(m))
            {
                //for now show encoded message
                //TODO: use factory class to create gui from messages
                std::stringstream s;
                s << m;

                text_message* t = new text_message{s.str()};
                add(t);
            }
        }
    }
}
