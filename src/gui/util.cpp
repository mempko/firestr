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

#include "gui/util.hpp"
#include "util/env.hpp"

#include <QFileDialog>

#include <fstream>

namespace u = fire::util;

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
    }
}
