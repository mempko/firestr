#include <QtWidgets>

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
            _layout->setAlignment(Qt::AlignTop);

            //setup base
            setWidgetResizable(true);
            setWidget(_root);

            QObject::connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            INVARIANT(_root);
            INVARIANT(_layout);
        }

        void delete_layout_item(QLayoutItem* l)
        {
            REQUIRE(l);
            REQUIRE(l->widget());
            delete l->widget();
            delete l;
        }

        void list::clear() 
        {
            INVARIANT(_layout);

            QLayoutItem *c = 0;
            while((c = _layout->takeAt(0)) != 0)
                delete_layout_item(c);

            ENSURE_EQUAL(_layout->count(), 0);
        }

        void list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            _layout->addWidget(w);
        }

        void list::remove(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);
            auto i = _layout->indexOf(w);
            if(i == -1) return;

            remove(i);
        }

        void list::remove(size_t i)
        {
            REQUIRE_RANGE(i , 0, size());
            INVARIANT(_layout);

            auto l = _layout->takeAt(i);
            CHECK(l);

            delete_layout_item(l);
        }

        QWidget* list::get(size_t i) const
        {
            REQUIRE_RANGE(i , 0, size());
            INVARIANT(_layout);

            auto l = _layout->itemAt(i);
            CHECK(l);

            auto w = l->widget();
            ENSURE(w);
            return w;
        }

        size_t list::size() const
        {
            INVARIANT(_layout);
            return _layout->count();
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
