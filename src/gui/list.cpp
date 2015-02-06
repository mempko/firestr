/*
 * Copyright (C) 2014  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * Botan library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than Botan. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

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
            if(del_widget && l->widget()) 
                delete l->widget();
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

            _just_added = true;
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
            if(!_just_added) return;
            _just_added = false;
            if(_auto_scroll) verticalScrollBar()->setValue(max);
        }
    }
}
