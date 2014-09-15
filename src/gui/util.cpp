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
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#include "gui/util.hpp"
#include "gui/app/app_service.hpp"
#include "util/env.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QInputDialog>

#include <fstream>

namespace u = fire::util;
namespace a = fire::gui::app;

namespace fire
{
    namespace gui
    {
        std::string convert(const QString& q)
        {
            return q.toUtf8().constData();
        }

        const unsigned short* convert16(const QString& q)
        {
            return q.utf16();
        }

        std::string app_id(const user::local_user& l)
        {
            return "firestr-" + l.info().id();
        }

        std::string get_file_name(QWidget* root)
        {
            REQUIRE(root);
            auto home = u::get_home_dir();
            auto file = QFileDialog::getOpenFileName(root, "Open File", home.c_str());
            auto sf = convert(file);
            return sf;
        }

        bool load_from_file(const std::string& f, u::bytes& data)
        {
            std::ifstream fs(f.c_str(), std::fstream::in | std::fstream::binary);
            if(!fs) return false;

            fs.seekg (0, fs.end);
            size_t length = fs.tellg();
            fs.seekg (0, fs.beg);

            data.resize(length);
            fs.read(data.data(), length);

            return true;
        }

        QPushButton* make_x_button()
        {
            auto b = new QPushButton("x");
            b->setMaximumSize(20,20);
            b->setMinimumSize(20,20);
            b->setStyleSheet("border: 0px; background-color: 'light grey'; color: 'red';");

            ENSURE(b);
            return b;
        }

        bool install_app_gui(a::app& a, a::app_service& s, QWidget* w)
        {
            REQUIRE(w);

            bool exists = s.available_apps().count(a.id());
            bool overwrite = false;
            if(exists)
            {
                QMessageBox q(w);
                q.setText("Update App?");
                q.setInformativeText("App already exists in your collection, update it?");
                auto *ub = q.addButton(w->tr("Update"), QMessageBox::ActionRole);
                auto *cb = q.addButton(w->tr("New Version"), QMessageBox::ActionRole);
                auto *canb = q.addButton(QMessageBox::Cancel);
                auto ret = q.exec();
                if(q.clickedButton() == canb) return false;

                overwrite = q.clickedButton() == ub;
            } 

            if(!overwrite)
            {
                QString curr_name = a.name().c_str();
                bool ok;
                auto g = QInputDialog::getText(w, w->tr("Install App"),
                        w->tr("App Name:"), QLineEdit::Normal, curr_name, &ok);

                if (!ok || g.isEmpty()) return false;

                std::string name = gui::convert(g);
                a.name(name);
            }

            if(!overwrite && exists) s.clone_app(a);
            else s.save_app(a);

            return true;
        }

    }
}
