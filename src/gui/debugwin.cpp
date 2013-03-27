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

#include "util/log.hpp"

#include <fstream>

#include <QVBoxLayout>
#include <QTabWidget>
#include <QGridLayout>
#include <QFileInfo>
#include <QTimer>

namespace us = fire::user;
namespace s = fire::session;

namespace fire
{
    namespace gui
    {
        namespace 
        {
            const std::size_t UPDATE_LOG = 1000;

        }
        debug_win::debug_win(
                us::user_service_ptr us, 
                s::session_service_ptr ss, 
                QWidget* parent) :
            _user_service{us},
            _session_service{ss},
            _log_last_file_pos{0},
            QDialog{parent}
        {
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
            tabs->addTab(log_tab, "log");
            tabs->addTab(mailbox_tab, "mailboxes");

            update_log();

            auto *t = new QTimer(this);
            connect(t, SIGNAL(timeout()), this, SLOT(update_log()));
            t->start(UPDATE_LOG);

            ENSURE(_log);
            ENSURE(_user_service);
            ENSURE(_session_service);
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
