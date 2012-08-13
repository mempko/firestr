#include <QtGui>

#include "gui/message.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace gui
    {
        message::message()
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QGridLayout{_root};
            _layout->setSizeConstraint(QLayout::SetMinimumSize);

            //setup base
            setWidgetResizable(true);
            setWidget(_root);

            INVARIANT(_root);
            INVARIANT(_layout);
        }

        message::~message()
        {
            INVARIANT(_root);
            INVARIANT(_layout);
        }

        const QWidget* message::root() const 
        {
            INVARIANT(_root);
            return _root;
        }

        QWidget* message::root()
        {
            INVARIANT(_root);
            return _root;
        }

        const QGridLayout* message::layout() const 
        {
            INVARIANT(_layout);
            return _layout;
        }

        QGridLayout* message::layout() 
        {
            INVARIANT(_layout);
            return _layout;
        }
    }
}
