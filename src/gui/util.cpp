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

#include "gui/util.hpp"
#include "gui/app/app_service.hpp"
#include "util/env.hpp"
#include "util/log.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QFontDatabase>
#include <QDialogButtonBox>

#include <fstream>

#include "gui/data/entypo.dat"

namespace u = fire::util;
namespace a = fire::gui::app;

namespace fire
{
    namespace gui
    {
        namespace
        {
            bool GUI_SETUP_CALLED = false;
        }

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

        void make_small(QWidget& b)
        {
            b.setFont(QFont{"Entypo", 26});
            b.setMaximumSize(24,24);
            b.setMinimumSize(24,24);
        }

        void make_big(QWidget& b)
        {
            b.setFont(QFont{"Entypo", 40});
            b.setMaximumSize(40,40);
            b.setMinimumSize(40,40);
        }

        void make_big_centered(QWidget& b)
        {
            b.setFont(QFont{"Entypo", 40});
            b.setMaximumHeight(40);
            b.setMinimumHeight(40);
        }

        void make_bigger_centered(QWidget& b)
        {
            b.setFont(QFont{"Entypo", 80});
            b.setMaximumHeight(80);
            b.setMinimumHeight(80);
        }

        void make_x(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_small(b);
            b.setText("\xE2\x9c\x96");  //\u2716
            b.setStyleSheet("border: 0px; color: 'red';");
        }

        QPushButton* make_x_button()
        {
            auto b = new QPushButton;
            make_x(*b);

            ENSURE(b);
            return b;
        }

        void make_plus(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xe2\x8a\x95"); //\u2295
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_minimize(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xe2\x8a\x9f"); //\u229F
            b.setStyleSheet("border: 0px; color: 'grey';");
        }

        void make_maximize(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xe2\x8a\x9e"); //\u229E
            b.setStyleSheet("border: 0px; color: 'grey';");
        }

        void make_install(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xee\x9d\xb8"); //\uE778
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_reply(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xee\x9c\x92"); //\uE712
            b.setStyleSheet("border: 0px; color: 'black';");
        }

        void make_export(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big(b);
            b.setText("\xee\x9c\x95"); //\uE715
            b.setStyleSheet("border: 0px; color: 'black';");
        }

        void make_save(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big(b);
            b.setText("\xf0\x9f\x92\xbe"); //\U0001F4BE
            b.setStyleSheet("border: 0px; color: 'black';");
        }

        void make_add_contact_small(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xee\x9c\x80"); //\uE700
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_add_contact(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xee\x9c\x80"); //\uE700
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_big_add_contact(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_bigger_centered(b);
            b.setText("\xee\x9c\x80"); //\uE700
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_big_email(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_bigger_centered(b);
            b.setText("\xe2\x9c\x89"); //\u2709
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_big_identity(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_bigger_centered(b);
            b.setText("\xee\x9c\xa2"); //\uE722
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_cancel(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xe2\x9d\x8c"); //\u274C
            b.setStyleSheet("border: 0px; color: 'red';");
        }

        void make_ok(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xe2\x9c\x93"); //\u2713
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_add_to_list(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xee\x80\x83"); //\uE003
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_introduce(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xf0\x9f\x91\xa5"); //\U0001F465
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_new_conversation(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_big_centered(b);
            b.setText("\xee\x9c\xa0"); //\uE720
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_new_conversation_small(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(b);
            b.setText("\xee\x9c\xa0"); //\uE720
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_big_new_conversation(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_bigger_centered(b);
            b.setText("\xee\x9c\xa0"); //\uE720
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_progress_00(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_small(w);
            w.setStyleSheet("border: 0px; color: 'black';");
        }

        void make_progress_0(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_progress_00(w);
            w.setText("\xee\x9d\xa8"); //\uE768
        }

        void make_progress_1(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_progress_00(w);
            w.setText("\xee\x9d\xa9"); //\uE769
        }

        void make_progress_2(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_progress_00(w);
            w.setText("\xee\x9d\xaa"); //\uE76A
        }

        void make_progress_3(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_progress_00(w);
            w.setText("\xee\x9d\xab"); //\uE76B
        }

        void make_error(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_small(w);
            w.setText("\xe2\x9a\xa0"); //\u26A0
            w.setStyleSheet("border: 0px; color: 'red';");
        }

        void make_thumbs_up(QLabel& w)
        {
            REQUIRE(GUI_SETUP_CALLED);
            make_small(w);
            w.setText("\xf0\x9f\x91\x8d"); //\U0001F44D
            w.setStyleSheet("border: 0px; color: 'green';");
        }

        void make_next(QPushButton& b)
        {
            REQUIRE(GUI_SETUP_CALLED);

            make_bigger_centered(b);
            b.setText("\xee\x9d\x9a"); //\uE75A
            b.setStyleSheet("border: 0px; color: 'green';");
        }

        void enable_icon_button(QPushButton& b, bool enabled)
        {
            b.setEnabled(enabled);
            if(enabled) b.setStyleSheet("border: 0px; color: 'green';");
            else b.setStyleSheet("border: 0px; color: 'grey';");
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
                q.addButton(w->tr("New Version"), QMessageBox::ActionRole);
                auto *canb = q.addButton(QMessageBox::Cancel);
                q.exec();
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

        void setup_entypo()
        {
            auto entypo = QByteArray::fromRawData(reinterpret_cast<const char*>(data::entypo_ttf), data::entypo_ttf_len);
            auto id = QFontDatabase::addApplicationFontFromData(entypo);

            for(auto l : QFontDatabase::applicationFontFamilies(id))
                LOG << "loaded font: " << convert(l) << " id: " << id << std::endl;
        }

        void setup_gui()
        {
            REQUIRE_FALSE(GUI_SETUP_CALLED);

            setup_entypo();
            GUI_SETUP_CALLED = true;

            ENSURE(GUI_SETUP_CALLED);
        }

        std::string color_to_stylesheet(const QColor c)
        {
            std::stringstream s;
            s << "color: rgba(" << c.red() << "," << c.green() << ","<< c.blue() << ","<<c.alpha() << ");";
            return s.str();
        }

        std::string background_color_to_stylesheet(const QColor c)
        {
            std::stringstream s;
            s << "background-color: rgba(" << c.red() << "," << c.green() << ","<< c.blue() << ","<<c.alpha() << ");";
            return s.str();
        }

        std::string color_to_stylesheet_2(const QColor c)
        {
            std::stringstream s;
            s << "rgba(" << c.red() << "," << c.green() << ","<< c.blue() << ","<<c.alpha() << ")";
            return s.str();
        }

        assert_dialog::assert_dialog(const char* msg, QWidget* parent) : QDialog{parent}
        {
            auto layout = new QVBoxLayout{this};
            setLayout(layout);

            auto l = new QLabel{tr(
                    "<h1>A <font color='red'>bug</font> was detected with the software!</h1><br/>"
                    "Fire★ is now going to close in case there was a security problem.<br><br>"
                    "Copy and Paste the text below and report this problem to<br><a href='mailto:firestr@librelist.com'>firestr@librelist.com</a><br> or <br><a href='https://github.com/mempko/firestr/issues'>https://github.com/mempko/firestr/issues</a>")};
            auto m = new QTextEdit;
            m->setPlainText(msg);
            m->setReadOnly(true); 

            auto b = new QDialogButtonBox(QDialogButtonBox::Ok);
            connect(b, SIGNAL(accepted()), this, SLOT(accept()));

            layout->addWidget(l);
            layout->addWidget(m);
            layout->addWidget(b);
        }
    }
}
