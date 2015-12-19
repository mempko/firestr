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
                const size_t PADDING = 30;
                const size_t TEXT_PADDING = 2;
            }

            generic_app::generic_app() : message{}
            {
                connect(this, SIGNAL(do_resize_hack()), SLOT(wacky_resize_hack()));
            }

            void generic_app::set_title(const std::string& t)
            {
                _title_text = t;
                if(_win) _win->setWindowTitle(_title_text.c_str());
            }

            const std::string& generic_app::title_text() const
            {
                return _title_text;
            }

            void generic_app::set_main(QWidget* m)
            {
                REQUIRE(m);
                _main = m;
                ENSURE(_main);
            }

            void generic_app::set_sub_window(QMdiSubWindow* w)
            {
                REQUIRE(w);
                REQUIRE(w->widget() == this);

                _win = w;

                ENSURE(_win);
            }

            void generic_app::alerted()
            {
                INVARIANT(_win);

                set_alert();

                if(_win->isMinimized()) 
                    _win->showNormal();
            }

            void generic_app::adjust_size()
            {
                INVARIANT(_win);
                INVARIANT(_main);
                emit do_resize_hack();
            }

            void generic_app::wacky_resize_hack()
            {
                auto aw = _win->mdiArea()->activeSubWindow();
                if(_win->isMaximized()) return;

                _win->showMinimized();
                _win->updateGeometry();
                _win->showNormal();
                _win->updateGeometry();
                _win->update();
                _win->mdiArea()->setActiveSubWindow(aw);
                emit did_resize_hack();
            }

            void generic_app::set_alert_style(const std::string& s)
            {
                INVARIANT(_main);
                _main->setStyleSheet(s.c_str());
            }
        }
    }
}
