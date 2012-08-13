#include <QtGui>

#include "gui/messagelist.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace gui
    {
        message_list::message_list()
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QVBoxLayout{_root};
            
            //setup scrollbar
            _scrollbar = verticalScrollBar();
            QObject::connect(_scrollbar, SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            //setup base
            setWidgetResizable(true);
            setWidget(_root);

            INVARIANT(_root);
            INVARIANT(_layout);
            INVARIANT(_scrollbar);
        }

        void message_list::clear() 
        {
            INVARIANT(_layout);

            QLayoutItem *c = 0;
            while((c = _layout->takeAt(0)) != 0)
            {
                CHECK(c);
                CHECK(c->widget());

                delete c->widget();
                delete c;
            } 

            ENSURE_EQUAL(_layout->count(), 0);
        }

        void message_list::add(message* m)
        {
            REQUIRE(m);
            INVARIANT(_layout);

            _layout->addWidget(m);
        }

        void message_list::scroll_to_bottom(int min, int max)
        {
            Q_UNUSED(min);
            INVARIANT(_scrollbar);
            _scrollbar->setValue(max);
        }
    }
}
