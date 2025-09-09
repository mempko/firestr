/*
 * Copyright (C) 2025  Maxim Noah Khailo
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
 */

#include "gui/app/qml_generic_app.hpp"

namespace fire
{
    namespace gui
    {
        namespace app
        {
            qml_generic_app::qml_generic_app(QObject* parent) :
                QObject{parent},
                _visible{true},
                _alert_set{false}
            {
            }

            qml_generic_app::~qml_generic_app()
            {
            }

            QString qml_generic_app::title() const
            {
                return _title;
            }

            void qml_generic_app::setTitle(const QString& title)
            {
                if(_title != title)
                {
                    _title = title;
                    emit titleChanged();
                }
            }

            bool qml_generic_app::visible() const
            {
                return _visible;
            }

            void qml_generic_app::setVisible(bool visible)
            {
                if(_visible != visible)
                {
                    _visible = visible;
                    emit visibleChanged();
                }
            }

            QString qml_generic_app::alertStyle() const
            {
                return _alert_style;
            }

            void qml_generic_app::setAlertStyle(const QString& style)
            {
                if(_alert_style != style)
                {
                    _alert_style = style;
                    emit alertStyleChanged();
                }
            }

            void qml_generic_app::alert()
            {
                if(!_alert_set)
                {
                    _alert_set = true;
                    setAlertStyle("border: 2px solid red;");
                    emit alerted();
                }
            }

            void qml_generic_app::clearAlert()
            {
                if(_alert_set)
                {
                    _alert_set = false;
                    setAlertStyle("");
                }
            }
        }
    }
}