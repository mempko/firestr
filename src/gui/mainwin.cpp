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

#include "gui/mainwin.hpp"
#include "gui/message.hpp"
#include "gui/textmessage.hpp"
#include "util/mencode.hpp"
#include "network/message_queue.hpp"

namespace fire
{
    namespace gui
    {
        main_window::main_window() :
            _about_action(0),
            _close_action(0),
            _main_menu(0),
            _root(0),
            _layout(0),
            _messages(0)
        {
            create_actions();
            create_main();
            create_menus();

            INVARIANT(_main_menu);
            INVARIANT(_about_action);
            INVARIANT(_close_action);
        }

        void main_window::create_main()
        {
            REQUIRE_FALSE(_root);
            REQUIRE_FALSE(_layout);
            REQUIRE_FALSE(_messages);
            
            //setup main
            _root = new QWidget{this};
            _layout = new QVBoxLayout{_root};

            //setup message list
            _messages = new message_list;
            _layout->addWidget(_messages);

            //setup base
            setWindowTitle(tr("Firestr"));
            setCentralWidget(_root);

            ENSURE(_root);
            ENSURE(_layout);
            ENSURE(_messages);
        }

        void main_window::create_menus()
        {
            REQUIRE_FALSE(_main_menu);
            REQUIRE(_about_action);
            REQUIRE(_close_action);
            REQUIRE(_test_action);

            _main_menu = new QMenu(tr("&Main"), this);
            _main_menu->addAction(_about_action);
            _main_menu->addAction(_test_action);
            _main_menu->addSeparator();
            _main_menu->addAction(_close_action);

            menuBar()->addMenu(_main_menu);

            ENSURE(_main_menu);
        }
        
        void main_window::create_actions()
        {
            REQUIRE_FALSE(_about_action);
            REQUIRE_FALSE(_close_action);

            _about_action = new QAction(tr("&About"), this);
            connect(_about_action, SIGNAL(triggered()), this, SLOT(about()));

            _close_action = new QAction(tr("&Exit"), this);
            connect(_close_action, SIGNAL(triggered()), this, SLOT(close()));

            _test_action = new QAction(tr("&Test"), this);
            connect(_test_action, SIGNAL(triggered()), this, SLOT(test()));

            ENSURE(_about_action);
            ENSURE(_close_action);
            ENSURE(_test_action);
        }


        void main_window::about()
        {
            QMessageBox::about(this, tr("About Firestr"),
                tr("<p><b>Firestr</b> allows you to communicate with people via multimedia "
                    "applications. Write, close, modify, and send people programs which "
                    "communicate with each other automatically, in a distributed way.</p>"
                    "<p>This is not the web, but it is on the internet.<br> "
                    "This is not a chat program, but a way for programs to chat.<br> "
                    "This is not a way to share code, but a way to share running software.</p> "
                    "<p>This program is created by <b>Maxim Noah Khailo</b> and is liscensed as GPLv3</p>"));
        }

        void main_window::test()
        {
            INVARIANT(_messages);

            //test text message
            for(int i = 0; i < 3; i++)
            {
                std::stringstream m;
                m << "Test Message: " << i;
                text_message* t = new text_message{m.str()};
                _messages->add(t);
            }

            try
            {
                //test dict
                util::dict d;
                d["a"] = 3;
                d["b"] = "ho";

                util::array f;
                f.add("orange");
                f.add("apple");

                d["fruit"] = f;

                std::stringstream m;
                m << d;

                text_message* t = new text_message{m.str()};
                _messages->add(t);

                util::dict d2;
                m >> d2;

                std::stringstream m2;
                m2 << d2;

                text_message* t2 = new text_message{m2.str()};
                _messages->add(t2);

                //test message queue
                network::message_queue_ptr q1 = network::create_message_queue("zmq,tcp://127.0.0.1:8080,bnd,psh");
                CHECK(q1);

                network::message_queue_ptr q2 = network::create_message_queue("zmq,tcp://127.0.0.1:8080,con,pul");
                CHECK(q2);

                std::string hi = "hi";
                q1->send(hi);
                text_message* t3 = new text_message{"sent: " + hi};
                _messages->add(t3);

                std::string hi2;
                q2->recieve(hi2);
                text_message* t4 = new text_message{"got: " + hi2};
                _messages->add(t4);

                namespace u = util;
                u::dict d5{
                    {"ooh", 
                            u::array{
                                34, 
                                "hi", 
                                u::array{ 1,2,2 }, 
                                u::dict{{"test",45}}}
                    },
                    {"moop", 34}
                }; 
                std::stringstream m5;
                m5 << d5;

                text_message* t5 = new text_message{m5.str()};
                _messages->add(t5);
            }
            catch(std::exception& e)
            {
                text_message* t = new text_message{e.what()};
                _messages->add(t);
            }
        }
    }
}
