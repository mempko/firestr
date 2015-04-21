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

#ifndef FIRESTR_GUI_APP_GENERIC_APP_H
#define FIRESTR_GUI_APP_GENERIC_APP_H

#include "gui/message.hpp"

#include <QPushButton>
#include <QLabel>
#include <QMdiSubWindow>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class generic_app : public message
            {
                Q_OBJECT
                public:
                    generic_app();

                    virtual void start() = 0;
                    virtual void contact_quit(const std::string& id) = 0;

                public:
                    const std::string& title_text() const;
                    void set_sub_window(QMdiSubWindow*);

                protected:
                    virtual void set_alert_style(const std::string& s);

                protected:
                    void set_title(const std::string&);
                    void set_main(QWidget*);
                    void adjust_size();
                    void alerted();

                signals:
                    void do_maximize();
                    void do_minimize();

                private slots:
                    void minimize();
                    void maximize();

                private:
                    QMdiSubWindow* _win = nullptr;
                    QWidget* _main = nullptr;
                    std::string _title_text;
                    bool _visible = true;
            };
        }
    }
}

#endif

