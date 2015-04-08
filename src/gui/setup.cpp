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

#include "gui/setup.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QtWidgets>

namespace us = fire::user;
namespace sc = fire::security;

namespace fire
{
    namespace gui
    {
        us::local_user_ptr make_new_user(const std::string& home)
        {
            QMessageBox::information(
                    0, 
                    qApp->tr("Welcome"),
                    qApp->tr(
                        "Welcome to Fire★,<br>"
                        "First you will need to create a User Account.<br>"
                        "It is stored ONLY on this computer and the password is used to protect your identity.<br>"));
            bool ok = false;
            std::string name = "your name here";

            auto r = QInputDialog::getText(
                    0, 
                    qApp->tr("New User"), 
                    qApp->tr("Choose a User Name"),
                    QLineEdit::Normal, name.c_str(), &ok);

            if(ok && !r.isEmpty()) name = convert(r);
            else return {};

            bool pass_done = false;

            std::string pass = "";
            do 
            {
                auto p = QInputDialog::getText(
                        0, 
                        qApp->tr("Create Password"),
                        qApp->tr("Choose a Password"),
                        QLineEdit::Password, pass.c_str(), &ok);

                if(ok && !p.isEmpty()) pass = convert(p);
                else return {};

                std::string pass2 = "";
                auto p2 = QInputDialog::getText(
                        0, 
                        qApp->tr("Verify Password"),
                        qApp->tr("Type Password Again"),
                        QLineEdit::Password, pass2.c_str(), &ok);

                if(ok && !p2.isEmpty()) pass2 = convert(p2);
                else return {};

                pass_done = pass == pass2;

                if(!pass_done)
                    QMessageBox::warning(0, qApp->tr("Try Again"), qApp->tr("The passwords did not match, try again."));

            } while(!pass_done);

            auto key = std::make_shared<sc::private_key>(pass);
            auto user = std::make_shared<us::local_user>(name, key);
            us::save_user(home, *user);

            ENSURE(user);
            return user;
        }

        us::local_user_ptr load_user(const std::string& home)
        {
            //loop if wrong password
            //load user throws error if the pass is wrong
            bool error = true;
            while(error)
            try
            {
                bool ok = false;
                std::string pass = "";

                auto p = QInputDialog::getText(
                        0, 
                        qApp->tr("Enter Password"),
                        qApp->tr("Password"),
                        QLineEdit::Password, pass.c_str(), &ok);

                if(ok && !p.isEmpty()) pass = convert(p);
                else return {};

                auto user = us::load_user(home, pass);
                error = false;
                return user;
            }
            catch(std::exception& e) 
            {
                LOG << "Error loading user: " << e.what() << std::endl;
            }

            return {};
        }

        us::local_user_ptr setup_user(const std::string& home)
        {
            return us::user_created(home) ? load_user(home) : make_new_user(home);
        }

    }
}
