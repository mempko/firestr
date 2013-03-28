/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#include "gui/debugwin.hpp"

#include "gui/list.hpp"

#include "util/log.hpp"

#include <fstream>

#include <QVBoxLayout>
#include <QTabWidget>
#include <QGridLayout>
#include <QFileInfo>
#include <QTimer>

namespace us = fire::user;
namespace s = fire::session;
namespace m = fire::message;

namespace fire
{
    namespace gui
    {
        namespace 
        {
            const std::size_t UPDATE_GRAPH = 100;
            const std::size_t UPDATE_LOG = 1000;
            const std::size_t UPDATE_MAILBOXES = 1000;
            const std::size_t GRAPH_WIDTH = 500;
        }

        void init_scene(QGraphicsView& v)
        {
            auto s = new QGraphicsScene;
            //s->setSceneRect(v.visibleRegion().boundingRect());
            v.setScene(s);
            ENSURE(v.scene());
        }

        mailbox_debug::mailbox_debug(m::mailbox_wptr m) :
            _mailbox{m},
            _in_max{0},
            _out_max{0},
            _x{0},
            _prev_in_y{0},
            _prev_out_y{0}
        {
            auto* layout = new QGridLayout{this};
            setLayout(layout);

            auto mb = m.lock();
            if(!mb) return;

            auto label = new QLabel{mb->address().c_str()};
            layout->addWidget(label, 0,0,2,1);

            _in_graph = new QGraphicsView;
            _out_graph = new QGraphicsView;

            layout->addWidget(_in_graph, 0,1,1,3);
            layout->addWidget(_out_graph, 1,1,1,3);

            init_scene(*_in_graph);
            init_scene(*_out_graph);

            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update_graph()));
            t->start(UPDATE_GRAPH);
        }

        void draw_graph(QGraphicsView& v, int px, int py, int x, int y, size_t& max_y, const QPen& pen)
        {
            REQUIRE(v.scene());
            if(y > max_y) max_y = y;

            auto s = v.scene();
            s->addLine(px, -py, x, -y, pen);

            v.fitInView(x-GRAPH_WIDTH,-int(max_y),GRAPH_WIDTH, max_y); 
        }

        void mailbox_debug::update_graph()
        {
            INVARIANT(_in_graph);
            INVARIANT(_out_graph);

            auto mb = _mailbox.lock();
            if(!mb) return;

            auto px = _x;
            _x++;

            auto in_y = mb->in_size();
            auto out_y = mb->out_size();

            draw_graph(*_in_graph, px, _prev_in_y, _x, in_y, _in_max, QPen{QColor{"red"}});
            draw_graph(*_out_graph, px, _prev_out_y, _x, out_y, _out_max, QPen{QColor{"green"}});
                    
            _prev_in_y = in_y;
            _prev_out_y = out_y;
        }

        debug_win::debug_win(
                m::post_office_ptr p,
                us::user_service_ptr us, 
                s::session_service_ptr ss, 
                QWidget* parent) :
            _post{p},
            _user_service{us},
            _session_service{ss},
            _log_last_file_pos{0},
            _total_mailboxes{0},
            QDialog{parent}
        {
            REQUIRE(p);
            REQUIRE(us);
            REQUIRE(ss);

            auto* layout = new QVBoxLayout{this};
            setLayout(layout);
            auto* tabs = new QTabWidget{this};
            layout->addWidget(tabs);

            auto* log_tab = new QWidget;
            auto* log_layout = new QGridLayout{log_tab};

            _log = new QTextEdit;
            log_layout->addWidget(_log, 0,0);

            auto* mailbox_tab = new QWidget;
            auto* mailbox_layout = new QGridLayout{mailbox_tab};
            _mailboxes = new list;
            mailbox_layout->addWidget(_mailboxes, 0,0);

            tabs->addTab(log_tab, "log");
            tabs->addTab(mailbox_tab, "mailboxes");

            update_mailboxes();
            update_log();

            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update_log()));
            t->start(UPDATE_LOG);

            auto *t2 = new QTimer(this);
            connect(t2, SIGNAL(timeout()), this, SLOT(update_mailboxes()));
            t2->start(UPDATE_MAILBOXES);

            ENSURE(_log);
            ENSURE(_post);
            ENSURE(_user_service);
            ENSURE(_session_service);
        }

        void add_mailboxes(m::post_office& o, list& l, added_mailboxes& added)
        {
            for(const auto& m : o.boxes())
            {
                if(added.count(m.first)) continue;

                auto mb = new mailbox_debug{m.second};
                l.add(mb);
                added.insert(m.first);
            }

            for(const auto& of : o.offices())
            {
                auto ofp = of.second.lock();
                if(!ofp) continue;

                add_mailboxes(*ofp, l, added);
            }
        }

        void debug_win::update_mailboxes()
        {
            INVARIANT(_post);
            INVARIANT(_mailboxes);

            if(_post->boxes().size() == _total_mailboxes) return;

            add_mailboxes(*_post, *_mailboxes, _added_mailboxes);

            _total_mailboxes = _post->boxes().size();
        }

        void debug_win::update_log()
        {
            INVARIANT(_log);

            const auto& log_file = LOG_PATH;
            auto modified = QFileInfo(log_file.c_str()).lastModified();
            if(_log_last_file_pos != 0 && _log_last_modified == modified) return;

            std::ifstream i(log_file.c_str());
            if(!i.good()) return;

            i.seekg(_log_last_file_pos);

            std::string l;
            while(std::getline(i, l))
            {
                std::cerr << l << std::endl;
                _log->append(l.c_str());
                _log_last_file_pos = i.tellg();
            }

            _log_last_modified = modified;
        }
    }
}
