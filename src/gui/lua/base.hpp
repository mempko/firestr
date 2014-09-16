/*
 * Copyright (C) 2014  Maxim Noah Khailo
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

#ifndef FIRESTR_APP_LUA_BASE_H
#define FIRESTR_APP_LUA_BASE_H

#include "gui/list.hpp"
#include "gui/message.hpp"
#include "conversation/conversation.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include "util/disk_store.hpp"

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

            class observable_ref : public basic_ref
            {
                public:
                    void set_name(const std::string&);
                    std::string get_name() const;

                public:
                    virtual void handle(const std::string& t,  const fire::util::value& event){};

                private:
                    std::string _name;
            };

            using observable_ref_name_map = std::unordered_map<std::string, int>;

            struct widget_ref : public observable_ref
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
                bool is_self = false;
            };
            contact_ref empty_contact_ref(lua_api& api);

            struct bin_data
            {
                util::bytes data;
                size_t get_size() const;
                char get(size_t i) const;
                void set(size_t i, char);
                bin_data sub(size_t p, size_t s) const;
                void append(const bin_data&);
                std::string to_str() const;
            };

            struct bin_file_data 
            {
                std::string name;
                bin_data data;
                bool good = false;
                std::string get_name() const;
                size_t get_size() const;
                bin_data get_data() const;
                bool is_good() const;
            };

            struct file_data 
            {
                std::string name;
                std::string data;
                bool good = false;
                std::string get_name() const;
                size_t get_size() const;
                std::string get_data() const;
                bool is_good() const;
            };

            extern const std::string EVENT_MESSAGE;
            class event_message
            {
                public:
                    event_message(
                            const std::string& obj, 
                            const std::string& type, 
                            const fire::util::value&, lua_api*);
                    event_message(const fire::message::message&, lua_api*);
                    operator fire::message::message() const;

                public:
                    const std::string& obj() const { return _obj;}
                    const std::string& type() const { return _type;}
                    const util::value& value() const { return _v;}

                private:
                    std::string _obj;
                    std::string _type;
                    util::value _v;
                    lua_api* _api;
            };

            extern const std::string SCRIPT_MESSAGE;
            class script_message
            {
                public:
                    script_message(lua_api*);
                    script_message(const fire::message::message&, lua_api*);
                    operator fire::message::message() const;

                public:
                    void not_robust();
                    const fire::util::value& get(const std::string&) const;
                    void set(const std::string&, const fire::util::value&);
                    bool has(const std::string&) const;
                    bin_data get_bin(const std::string&) const;
                    void set_bin(const std::string&, const bin_data&);
                    contact_ref from() const;
                    bool is_local() const;
                    app_ref app() const;
                    void set_type(const std::string&);
                    const std::string& get_type() const;

                private:
                    std::string _type;
                    std::string _from_id;
                    std::string _local_app_id;
                    util::dict _v;
                    bool _robust = true;
                    lua_api* _api;
            };

            class store_ref
            {
                public:
                    store_ref(util::disk_store&);

                public:
                    std::string get(const std::string&) const;
                    void set(const std::string&, const std::string&);

                    void set_bin(const std::string&, const bin_data&);
                    bin_data get_bin(const std::string&) const;
                    bool has(const std::string&) const;
                    bool remove(const std::string&);
                    size_t size() const;

                public:
                    const util::disk_store& store() const;
                    util::disk_store& store();

                private:
                    util::disk_store& _d;
            };

            int store_ref_set(lua_State* L);
            int store_ref_get(lua_State* L);
            int script_message_set(lua_State* L);
            int script_message_get(lua_State* L);
            using store_ref_ptr = std::unique_ptr<store_ref>;
        }
    }
}

#endif
