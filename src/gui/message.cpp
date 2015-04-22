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

#include "gui/message.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"

#include <QPropertyAnimation>

namespace fire
{
    namespace gui
    {
        namespace
        {
            const QColor MID_ALERT{128, 50, 50, 20};
            const QColor END_ALERT{128, 50, 50,0};
        }

        message::message()
        {
            //setup root
            _root = new QWidget;
            _root->setFocusPolicy(Qt::WheelFocus);

            //setup layout
            _layout = new QGridLayout{_root};
            _layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            _alert_color = END_ALERT;

            //setup base
            setWidgetResizable(true);
            setWidget(_root);
            setLayout(_layout);
            setFrameShape(QFrame::NoFrame);
            setContentsMargins(0,0,0,0);

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

        bool message::visible() const
        {
            INVARIANT(_root);
            return !_root->visibleRegion().isEmpty();
        }

        void message::set_alert() 
        {
            if(visible()) return;

            _alert_set = true;
            set_alert_color(MID_ALERT);
        }

        QColor message::alert_color() const
        {
            return _alert_color;
        }

        void message::clear_alert()
        {
            if(!visible()) return;
            if(!_alert_set) return;

            auto a = new QPropertyAnimation{this, "alert_color"};
            a->setDuration(600);
            a->setKeyValueAt(0.0, MID_ALERT);
            a->setKeyValueAt(1.0, END_ALERT);
            a->start(QAbstractAnimation::DeleteWhenStopped);

            _alert_set = false;
        }

        void message::set_alert_color(const QColor& c)
        {
            INVARIANT(_root);
            _alert_color = c;
            if(c.alpha() <= 1)
                set_alert_style("");
            else 
                set_alert_style(background_color_to_stylesheet(c).c_str());
        }
    }
}
