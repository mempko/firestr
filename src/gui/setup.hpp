/*
 * Copyright (C) 2015  Maxim Noah Khailo
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

#ifndef FIRESTR_GUI_SETUP_H
#define FIRESTR_GUI_SETUP_H

#include "user/user.hpp"

#include <string>

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

namespace fire
{
    namespace gui
    {
        class setup_user_dialog : public QDialog
        {
            Q_OBJECT
            public:
                setup_user_dialog(const std::string& home, QWidget* parent = nullptr);
            public:
                std::string name() const;
                std::string pass() const;
                bool should_create() const;

            public slots:
                void cancel();
                void create();
                void validate_name(QString);
                void validate_pass1(QString);
                void validate_pass2(QString);

            private:
                void validate_input();
                void keyPressEvent(QKeyEvent*);

            private:
                bool _valid_name = false;
                bool _valid_pass = false;
                bool _should_create = false;
                QLineEdit* _name = nullptr;
                QLineEdit* _pass1 = nullptr;
                QLineEdit* _pass2 = nullptr;
                QPushButton* _create = nullptr;
        };

        class login_dialog : public QDialog
        {
            Q_OBJECT
            public:
                login_dialog(const std::string& home, bool retry, QWidget* parent = nullptr);

            public:
                std::string pass() const;
                bool should_login() const;

            public slots:
                void cancel();
                void login();
                void validate_pass(QString);

            private:
                void validate_input();
                void keyPressEvent(QKeyEvent*);

            private:
                bool _should_login = false;
                QLineEdit* _pass = nullptr;
                QPushButton* _login = nullptr;
        };

        user::local_user_ptr setup_user(const std::string& home);
    }
}
#endif
