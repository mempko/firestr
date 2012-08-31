#include <QtGui>

#include "gui/messagelist.hpp"
#include "gui/unknown_message.hpp"
#include "gui/test_message.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;

namespace fire
{
    namespace gui
    {
        namespace
        {
            const size_t TIMER_SLEEP = 100;//in milliseconds
        }

        message_list::message_list(const std::string& name, us::user_service_ptr service) :
            _user_service{service},
            _name{name},
            _mail{new m::mailbox{name}}
        {
            REQUIRE(service);

            //setup scrollbar
            _scrollbar = verticalScrollBar();
            QObject::connect(_scrollbar, SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            //setup message timer
            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(check_mail()));
            t->start(TIMER_SLEEP);

            _sender.reset(new ms::sender{_user_service, _mail});

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_scrollbar);
            INVARIANT(_mail);
            INVARIANT(_user_service);
            INVARIANT(_sender);
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

        messages::sender_ptr message_list::sender()
        {
            ENSURE(_sender);
            return _sender;
        }

        void message_list::check_mail() 
        {
            INVARIANT(_mail);

            m::message m;
            while(_mail->pop_inbox(m))
            {
                //for now show encoded message
                //TODO: use factory class to create gui from messages
                if(m.meta.type == ms::TEST_MESSAGE)
                {
                    add(new test_message{m, _sender});
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
