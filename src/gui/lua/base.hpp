/*
 * Copyright (C) 2013  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vedit_refsion 3 of the License, or
 * (at your option) any later vedit_refsion.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FIRESTR_APP_LUA_BASE_H
#define FIRESTR_APP_LUA_BASE_H

#ifndef Q_MOC_RUN
#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#endif

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>
#include <QGraphicsView>

#include "slb/SLB.hpp"

#include <string>
#include <unordered_map>
#include <thread>

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            class lua_api;

            struct basic_ref
            {
                int id;
                lua_api* api;
            };

            struct widget_ref : public basic_ref
            {
                bool enabled(); 
                void enable();
                void disable();
            };

            class script_message;
            struct app_ref : public basic_ref
            {
                std::string app_id;
                std::string get_id() const;
                void send(const script_message&); 
            };
            app_ref empty_app_ref(lua_api& api);

            struct contact_ref : public basic_ref
            {
                std::string user_id;
                std::string get_id() const;
                std::string get_name() const;
                bool is_online() const;
            };
            contact_ref empty_contact_ref(lua_api& api);

            struct bin_data
            {
                util::bytes data;
                char get(size_t i) const;
                void set(size_t i, char);
                std::string to_str() const;
            };

            struct bin_file_data 
            {
                std::string name;
                bin_data data;
                bool good = false;
                std::string get_name() const;
                bin_data get_data() const;
                bool is_good() const;
            };

            struct file_data 
            {
                std::string name;
                std::string data;
                bool good = false;
                std::string get_name() const;
                std::string get_data() const;
                bool is_good() const;
            };

            extern const std::string SCRIPT_MESSAGE;
            class script_message
            {
                public:
                    script_message(lua_api*);
                    script_message(const fire::message::message&, lua_api*);
                    operator fire::message::message() const;

                public:
                    std::string get(const std::string&) const;
                    void set(const std::string&, const std::string&);
                    bin_data get_bin(const std::string&) const;
                    void set_bin(const std::string&, const bin_data&);
                    contact_ref from() const;
                    bool is_local() const;
                    app_ref app() const;

                private:
                    std::string _from_id;
                    std::string _local_app_id;
                    util::dict _v;
                    lua_api* _api;
            };
        }
    }
}

#endif
