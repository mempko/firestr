#include <QtWidgets>

#include "gui/list.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace fire
{
    namespace gui
    {
        list::list(QWidget* p) : QScrollArea{p}, _auto_scroll{false}
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QVBoxLayout{_root};
            _layout->setAlignment(Qt::AlignTop);
            _layout->setContentsMargins(0,0,0,0);

            //setup base
            setWidgetResizable(true);
            setWidget(_root);
            setContentsMargins(0,0,0,0);
            setFrameShape(QFrame::NoFrame);

            QObject::connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scroll_to_bottom(int, int)));

            INVARIANT(_root);
            INVARIANT(_layout);
        }

        void delete_layout_item(QLayoutItem* l, bool del_widget)
        {
            REQUIRE(l);
            if(del_widget && l->widget()) delete l->widget();
            delete l;
        }

        void list::clear(bool del_widgets) 
        {
            INVARIANT(_layout);

            QLayoutItem *c = nullptr;
            while((c = _layout->takeAt(0)) != nullptr)
                delete_layout_item(c, del_widgets);

            ENSURE_EQUAL(_layout->count(), 0);
        }

        void list::add(QWidget* w)
        {
            REQUIRE(w);
            INVARIANT(_layout);

            _layout->addWidget(w);
        }

        void list::remove(QWidget* w, bool del_widget)
        {
            REQUIRE(w);
            INVARIANT(_layout);
            auto i = _layout->indexOf(w);
            if(i == -1) return;

            remove(i, del_widget);
        }

        void list::remove(size_t i, bool del_widget)
        {
            REQUIRE_RANGE(i , 0, size());
            INVARIANT(_layout);

            auto l = _layout->takeAt(i);
            CHECK(l);

            delete_layout_item(l, del_widget);
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
