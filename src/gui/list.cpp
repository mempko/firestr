#include <QtGui>

#include "gui/list.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace fire
{
    namespace gui
    {
        list::list() : _auto_scroll{false}
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QVBoxLayout{_root};

            //setup base
            setWidgetResizable(true);
            setWidget(_root);

            QObject::connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            INVARIANT(_root);
            INVARIANT(_layout);
        }

        void list::clear() 
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

        void list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            _layout->addWidget(w);
        }

        void list::auto_scroll(bool v)
        {
            _auto_scroll = v;
        }

        void list::scroll_to_bottom(int min, int max)
        {
            Q_UNUSED(min);
            if(_auto_scroll) verticalScrollBar()->setValue(max);
        }
    }
}
