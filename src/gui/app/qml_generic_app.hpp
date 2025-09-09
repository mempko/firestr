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

#ifndef FIRESTR_GUI_APP_QML_GENERIC_APP_H
#define FIRESTR_GUI_APP_QML_GENERIC_APP_H

#include "message/mailbox.hpp"

#include <QObject>
#include <QString>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class qml_generic_app : public QObject
            {
                Q_OBJECT
                Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
                Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
                Q_PROPERTY(QString alertStyle READ alertStyle WRITE setAlertStyle NOTIFY alertStyleChanged)

                public:
                    qml_generic_app(QObject* parent = nullptr);
                    virtual ~qml_generic_app();

                    // Pure virtual methods that subclasses must implement
                    virtual void start() = 0;
                    virtual void contact_quit(const std::string& id) = 0;
                    virtual const std::string& id() const = 0;
                    virtual const std::string& type() const = 0;
                    virtual fire::message::mailbox_ptr mail() = 0;

                    // Title management
                    QString title() const;
                    void setTitle(const QString& title);

                    // Visibility management  
                    bool visible() const;
                    void setVisible(bool visible);

                    // Alert style management
                    QString alertStyle() const;
                    void setAlertStyle(const QString& style);

                signals:
                    void titleChanged();
                    void visibleChanged();
                    void alertStyleChanged();
                    void alerted();
                    void adjustSize();

                public slots:
                    void alert();
                    void clearAlert();

                protected:
                    QString _title;
                    bool _visible;
                    QString _alert_style;
                    bool _alert_set;
            };
        }
    }
}

#endif