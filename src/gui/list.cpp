#include <QtGui>

#include "gui/list.hpp"
#include "gui/textmessage.hpp"
#include "util/dbc.hpp"

#include <sstream>

namespace fire
{
    namespace gui
    {
        list::list(const std::string& name) :
            _name{name}
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QVBoxLayout{_root};

            //setup base
            setWidgetResizable(true);
            setWidget(_root);

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
    }
}
