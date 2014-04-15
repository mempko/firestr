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
 */

#ifndef FIRESTR_GUI_MESSAGE_H
#define FIRESTR_GUI_MESSAGE_H

#include <QScrollArea>
#include <QGridLayout>
#include <message/mailbox.hpp>

namespace fire
{
    namespace gui
    {
        class message : public QScrollArea
        {
            Q_OBJECT
            public:
                message();
                virtual ~message();

            public:
                virtual const std::string& id() const = 0;
                virtual const std::string& type() const = 0;
                virtual fire::message::mailbox_ptr mail() = 0;

            protected:
                const QWidget* root() const;
                QWidget* root() ;

                const QGridLayout* layout() const; 
                QGridLayout* layout(); 

            private:
                QWidget* _root;
                QGridLayout* _layout;
        };
    }

}

#endif

