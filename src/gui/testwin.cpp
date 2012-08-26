/*
 * Copyright (C) 2012  Maxim Noah Khailo
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
#include <sstream>
#include <stdexcept>

#include <QtGui>

#include "util/dbc.hpp"

#include "gui/testwin.hpp"
#include "gui/message.hpp"
#include "gui/textmessage.hpp"
#include "util/mencode.hpp"
#include "network/message_queue.hpp"
#include "message/master_post.hpp"
#include "util/bytes.hpp"

namespace m = fire::message;
namespace u = fire::util;
                
namespace fire
{
    namespace gui
    {
        namespace m = fire::message;

        test_window::test_window(
                        const std::string& host, 
                        const std::string& port,
                        const std::string& fire_port) :
            _about_action{0},
            _close_action{0},
            _main_menu{0},
            _root{0},
            _layout{0},
            _messages{0},
            _fire_port{fire_port}
        {
            setup_post(host, port);
            create_actions();
            create_main();
            create_menus();

            INVARIANT(_main_menu);
            INVARIANT(_about_action);
            INVARIANT(_close_action);
        }

        void test_window::setup_post(
                        const std::string& host, 
                        const std::string& port)
        {
            REQUIRE(!_master);

            _master.reset(new m::master_post_office{host, port});

            INVARIANT(_master);
        }

        void test_window::create_main()
        {
            REQUIRE_FALSE(_root);
            REQUIRE_FALSE(_layout);
            REQUIRE_FALSE(_messages);
            REQUIRE(_master);
            
            //setup main
            _root = new QWidget{this};
            _layout = new QGridLayout{_root};

            _ctr_root = new QWidget;
            _ctr_layout = new QGridLayout{_ctr_root};

            //setup message list
            _messages = new message_list{"test"};
            _master->add(_messages->mail());

            //setup to controls
            const std::string to_address = "*:" + _fire_port;
            _to = new QLineEdit{to_address.c_str()};
            _ctr_layout->addWidget(_to, 0, 0);

            //setup test message controls
            _message_edit = new QLineEdit{"hi"};
            _ctr_layout->addWidget(_message_edit, 1, 0);
            _send_text = new QPushButton{"send"};
            connect(_send_text, SIGNAL(clicked()), this, SLOT(send_text()));
            _ctr_layout->addWidget(_send_text, 1, 1);

            //setup main layout
            _layout->addWidget(_ctr_root, 0, 0);
            _layout->addWidget(_messages, 1, 0);

            //setup base
            setWindowTitle(tr("Firetest"));
            setCentralWidget(_root);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_messages);
        }

        void test_window::create_menus()
        {
            REQUIRE_FALSE(_main_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);

            _main_menu = new QMenu(tr("&Main"), this);
            _main_menu->addAction(_about_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            menuBar()->addMenu(_main_menu);

            ENSURE(_main_menu);
        }
        
        void test_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);

            _about_action = new QAction(tr("&About"), this);
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction(tr("&Exit"), this);
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            ENSURE(_about_action);
            ENSURE(_close_action);
        }

        void test_window::about()
        {
            QMessageBox::about(this, tr("About Firetest"),
                tr("<p><b>Firetest</b> simple program used to develop Firestr"
                    "<p>This program is created by <b>Maxim Noah Khailo</b> and is liscensed as GPLv3</p>"));
        }

        void test_window::send_text()
        {
            INVARIANT(_messages);
            INVARIANT(_to);
            INVARIANT(_message_edit);

            try
            {
                std::string to = _to->text().toUtf8().constData();
                std::string to_address = "zmq,tcp://" + to;
                std::string text = _message_edit->text().toUtf8().constData();

                m::message m;
                m.meta.to = {to_address, "main"};
                m.data = u::to_bytes(text);

                _messages->mail()->push_outbox(m);
            }
            catch(std::exception& e)
            {
                text_message* t = new text_message{e.what()};
                _messages->add(t);
            }
        }
    }
}
