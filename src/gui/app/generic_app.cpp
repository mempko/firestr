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

#include "gui/app/generic_app.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"

#include <QPropertyAnimation>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            namespace
            {
                const size_t PADDING = 45;
            }

            generic_app::generic_app() : message{}
            {
                REQUIRE_FALSE(_title);
                REQUIRE_FALSE(_show_hide);

                _title = new QLabel{};

                _show_hide = new QPushButton;
                make_minimize(*_show_hide);
                _show_hide->setToolTip(tr("minimize app"));
                connect(_show_hide, SIGNAL(clicked()), this, SLOT(toggle_visible()));
                
                layout()->addWidget(_show_hide, 0,0);
                layout()->addWidget(_title, 0,1);

                ENSURE(_show_hide);
                ENSURE(_visible);
            }

            void generic_app::set_title(const std::string& t)
            {
                INVARIANT(_title);
                _title->setText(t.c_str());
                _title_text = t;
            }

            void generic_app::set_main(QWidget* m)
            {
                REQUIRE(m);
                _main = m;
                ENSURE(_main);
            }

            void generic_app::alerted()
            {
                INVARIANT(_title);
                set_alert();

                if(_visible) return;

                std::stringstream s;
                s << "<font color='red'>" << _title_text << " ...</font>";
                _title->setText(s.str().c_str());

            }

            void generic_app::animate_min_height_to(int h)
            {
                auto a = new QPropertyAnimation{this, "app_min_height"};

                a->setDuration(500);
                a->setKeyValueAt(0.0, app_min_height());
                a->setKeyValueAt(1.0, h);
                a->setEasingCurve(QEasingCurve::OutExpo);
                a->start(QAbstractAnimation::DeleteWhenStopped);
            }

            int generic_app::app_min_height() const
            {
                return minimumHeight();
            }

            void generic_app::set_app_min_height(int h)
            {
                setMinimumHeight(h);

                if(_visible && h >= static_cast<int>(_min_height) - 1)
                {
                    //enable min/max constraint so that the widget can be to original size
                    layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);
                    setMaximumHeight(_max_height);
                    _main->show();
                }
            }

            void generic_app::toggle_visible()
            {
                REQUIRE(_main);
                INVARIANT(_show_hide);
                INVARIANT(root());
                INVARIANT(layout());

                if(_visible)
                {
                    {
                        std::stringstream s;
                        s << "<font color='grey'>" << _title_text << " ...</font>";
                        _title->setText(s.str().c_str());
                    }

                    make_maximize(*_show_hide);

                    //remove min/max constraint so that the widget can be resized to a tiny size
                    layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

                    _show_hide->setToolTip(tr("show app"));
                    _visible = false;
                    _min_height = minimumHeight();
                    _max_height = maximumHeight();
                    _main->hide();
                    animate_min_height_to(PADDING);
                    setMaximumHeight(PADDING);
                } 
                else
                {
                    _title->setText(_title_text.c_str());

                    make_minimize(*_show_hide);
                    _show_hide->setToolTip(tr("hide app"));
                    _visible = true;
                    animate_min_height_to(_min_height);
                }
            }

            void generic_app::set_alert_style(const std::string& s)
            {
                INVARIANT(_main);
                _main->setStyleSheet(s.c_str());
            }
        }
    }
}
