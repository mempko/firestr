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
#include "gui/icon.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QtWidgets>
#include <QFormLayout>

namespace us = fire::user;
namespace sc = fire::security;

namespace fire
{
    namespace gui
    {
        namespace
        {
            const int MARGIN = 40;
            const int LOGO_HEIGHT = 150;
        }

        QPixmap smaller_logo()
        {
            auto logo_pix = logo_pixmap();
            return logo_pix.scaledToHeight(LOGO_HEIGHT, Qt::SmoothTransformation);
        }

        setup_user_dialog::setup_user_dialog(const std::string& home, QWidget* parent) : QDialog{parent}
        {
            auto l = new QVBoxLayout{this};
            auto fw = new QWidget;
            auto f = new QFormLayout{fw};
            setLayout(l);

            auto logo = new QLabel;
            logo->setPixmap(smaller_logo());

            auto welcome = new QLabel{tr(
                        "<br><h1>The <font color='green'>Grass</font> Computing Platform</h1><br>")};

            auto create_iden = new QLabel{tr("<h3>Create Your <font color='blue'>Identity</font></h3>")};
            auto points = new QLabel{tr(
                        "<ul>"
                        "<li>The identity is stored on this computer.</li>"
                        "<li>Your identity and data is not stored anywhere else.</li>"
                        "<li>The password is used to protect your identity.</li>"
                        "</ul><br>")};

            _name = new QLineEdit;
            _pass1 = new QLineEdit;
            _pass1->setEchoMode(QLineEdit::Password);
            _pass2 = new QLineEdit;
            _pass2->setEchoMode(QLineEdit::Password);


            connect(_name, SIGNAL(textChanged(QString)), this, SLOT(validate_name(QString)));
            connect(_pass1, SIGNAL(textChanged(QString)), this, SLOT(validate_pass1(QString)));
            connect(_pass2, SIGNAL(textChanged(QString)), this, SLOT(validate_pass2(QString)));

            _create = new QPushButton;
            _create->setToolTip(tr("Create Identity"));
            make_next(*_create);
            enable_icon_button(*_create, false);
            connect(_create, SIGNAL(clicked()), this, SLOT(create()));


            f->addRow(tr("Name"), _name);
            f->addRow(tr("Password"), _pass1);
            f->addRow(tr("Repeat Password"), _pass2);
            f->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

            l->addWidget(logo);
            l->addWidget(welcome);
            l->addWidget(create_iden);
            l->addWidget(points);
            l->addWidget(fw);
            l->addWidget(_create);


            l->setAlignment(logo, Qt::AlignHCenter);
            l->setAlignment(welcome, Qt::AlignHCenter);
            l->setAlignment(create_iden, Qt::AlignHCenter);
            l->setAlignment(points, Qt::AlignHCenter);
            l->setAlignment(fw, Qt::AlignHCenter);
            l->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

            ENSURE(_name);
            ENSURE(_pass1);
            ENSURE(_pass2);
            ENSURE(_create);
        }

        //prevent enter from calling accept
        void setup_user_dialog::keyPressEvent(QKeyEvent* e)
        {
            REQUIRE(e);
            if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
            {
                if(!_should_create) return;
                else { create(); return;}
            }
            QDialog::keyPressEvent(e);
        }

        std::string setup_user_dialog::name() const
        {
            INVARIANT(_name);
            return convert(_name->text());
        }

        std::string setup_user_dialog::pass() const
        {
            INVARIANT(_pass1);
            return convert(_pass1->text());
        }

        bool setup_user_dialog::should_create() const
        {
            return _should_create;
        }

        void setup_user_dialog::validate_pass1(QString s)
        {
            INVARIANT(_pass1);
            _pass2->setText("");
            bool valid = !_pass1->text().isEmpty();
            if(valid) _pass1->setStyleSheet("QLineEdit { background-color: rgb(128, 255, 128) }");
            else      _pass1->setStyleSheet("QLineEdit { background-color: rgb(255, 128, 128) }");
        }

        void setup_user_dialog::validate_name(QString s)
        {
            INVARIANT(_name);

            _valid_name = !_name->text().isEmpty();
            if(_valid_name) _name->setStyleSheet("QLineEdit { background-color: rgb(128, 255, 128) }");
            else            _name->setStyleSheet("QLineEdit { background-color: rgb(255, 128, 128) }");

            validate_input();
        }

        void setup_user_dialog::validate_pass2(QString s)
        {
            INVARIANT(_pass1);
            INVARIANT(_pass2);
            INVARIANT(_create);

            auto pass1 = convert(_pass1->text());
            auto pass2 = convert(_pass2->text());

            _valid_pass = pass1 == pass2 && !pass1.empty();

            if(_valid_pass) _pass2->setStyleSheet("QLineEdit { background-color: rgb(128, 255, 128) }");
            else            _pass2->setStyleSheet("QLineEdit { background-color: rgb(255, 128, 128) }");

            validate_input();
        }

        void setup_user_dialog::validate_input()
        {
            _should_create = _valid_name && _valid_pass;
            enable_icon_button(*_create, _should_create);
        }

        void setup_user_dialog::cancel()
        {
            _should_create = false;
            reject();
        }

        void setup_user_dialog::create()
        {
            ENSURE(_should_create);
            accept();
        }

        login_dialog::login_dialog(const std::string& home, bool retry, QWidget* parent) : QDialog{parent}
        {
            auto l = new QVBoxLayout{this};
            auto fw = new QWidget;
            auto f = new QFormLayout{fw};
            setLayout(l);

            auto logo = new QLabel;
            logo->setPixmap(smaller_logo());

            auto welcome = new QLabel{tr(
                        "<br><h1>The <font color='darkgreen'>Grass</font> Computing Platform</h1><br>")};

            auto validate_iden = retry ? 
                new QLabel{tr("<h3><font color='red'>Invalid Password</font></h3><br>")}:
                new QLabel{tr("<h3>Validate Your <font color='blue'>Identity</font></h3><br>")};

            _pass = new QLineEdit;
            _pass->setEchoMode(QLineEdit::Password);

            connect(_pass, SIGNAL(textChanged(QString)), this, SLOT(validate_pass(QString)));

            _login = new QPushButton;
            _login->setToolTip(tr("Validate Identity"));
            make_next(*_login);
            enable_icon_button(*_login, false);
            connect(_login, SIGNAL(clicked()), this, SLOT(login()));


            f->addRow(tr("Password"), _pass);
            f->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

            l->addWidget(logo);
            l->addWidget(welcome);
            l->addWidget(validate_iden);
            l->addWidget(fw);
            l->addWidget(_login);


            l->setAlignment(logo, Qt::AlignHCenter);
            l->setAlignment(welcome, Qt::AlignHCenter);
            l->setAlignment(validate_iden, Qt::AlignHCenter);
            l->setAlignment(fw, Qt::AlignHCenter);
            l->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

            ENSURE(_pass);
            ENSURE(_login);
        }

        //prevent enter from calling accept
        void login_dialog::keyPressEvent(QKeyEvent* e)
        {
            REQUIRE(e);
            INVARIANT(_pass);
            if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
            {
                if(!_pass->text().isEmpty()) login();
                return;
            }
            QDialog::keyPressEvent(e);
        }

        void login_dialog::validate_pass(QString s)
        {
            INVARIANT(_pass);
            INVARIANT(_login);

            bool valid = !_pass->text().isEmpty();
            if(valid) 
            {
                enable_icon_button(*_login, true);
                _pass->setStyleSheet("QLineEdit { background-color: rgb(255, 255, 255) }");
            }
            else 
            {
                enable_icon_button(*_login, false);
                _pass->setStyleSheet("QLineEdit { background-color: rgb(255, 128, 128) }");
            }
        }

        std::string login_dialog::pass() const
        {
            INVARIANT(_pass);
            return convert(_pass->text());
        }

        bool login_dialog::should_login() const
        {
            return _should_login;
        }

        void login_dialog::cancel()
        {
            _should_login = false;
            reject();
        }

        void login_dialog::login()
        {
            _should_login = true;
            accept();
        }


        us::local_user_ptr make_new_user(const std::string& home)
        {
            setup_user_dialog d{home};
            d.exec();
            if(!d.should_create()) return us::local_user_ptr{};

            auto name = d.name();
            auto pass = d.pass();
            CHECK_FALSE(name.empty());
            CHECK_FALSE(pass.empty());

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
            bool retried = false;
            while(error)
            try
            {
                login_dialog login{home, retried};
                login.exec();
                if(!login.should_login()) return us::local_user_ptr{};
                auto pass = login.pass();

                auto user = us::load_user(home, pass);
                error = false;
                return user;
            }
            catch(std::exception& e) 
            {
                LOG << "Error loading user: " << e.what() << std::endl;
                retried = true;
            }

            return us::local_user_ptr{};
        }

        us::local_user_ptr setup_user(const std::string& home)
        {
            return us::user_created(home) ? load_user(home) : make_new_user(home);
        }

    }
}
