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

#include "gui/app/app.hpp"

#include "util/dbc.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/mencode.hpp"
#include "util/uuid.hpp"
#include "util/compress.hpp"

#include <boost/filesystem.hpp>

#include <fstream>
#include <stdexcept>

namespace u = fire::util;
namespace m = fire::message;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            namespace 
            {
                const std::string APP_MESSAGE = "app_message";
                const std::string METADATA_FILE = "metadata";
                const std::string CODE_FILE = "code.lua";
                const std::string DATA_PATH = "data";
                const std::string REMOVE_TAG_FILE = "removed";
            }

            std::string get_metadata_file(const std::string& dir)
            {
                bf::path p = dir;

                p /= METADATA_FILE;
                return p.string();
            }

            std::string get_code_file(const std::string& dir)
            {
                bf::path p = dir;

                p /= CODE_FILE;
                return p.string();
            }

            std::string get_data_path(const std::string& dir)
            {
                bf::path p = dir;

                p /= DATA_PATH;
                return p.string();
            }

            std::string get_remove_tag_path(const std::string& dir)
            {
                bf::path p = dir;

                p /= REMOVE_TAG_FILE;
                return p.string();
            }

            bool app_removed(const std::string& dir)
            {
                return bf::exists(get_remove_tag_path(dir));
            }

            bool tag_app_removed(const std::string& dir)
            {
                if(!bf::exists(dir)) return false;

                std::ofstream f(get_remove_tag_path(dir).c_str());
                if(!f.good()) return false;

                f << "1";
                return true;
            }

            app::app(u::disk_store& s, const std::string& id) : 
                _local_data(s), 
                _launched_local{true},
                _is_tmp{false}
            {
                REQUIRE_FALSE(id.empty());
                _meta.id = id;
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app(u::disk_store& s) : 
                _local_data(s), 
                _launched_local{true},
                _is_tmp{false}
            {
                _meta.id = u::uuid();
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app(
                    u::disk_store& s, 
                    const std::string& app_dir,
                    const m::message& m) : 
                _local_data(s), 
                _launched_local{false},
                _is_tmp{false}
            {
                REQUIRE_EQUAL(m.meta.type, APP_MESSAGE);

                _meta.path = app_dir;
                _meta.id = m.meta.extra["app_id"].as_string();
                _meta.name = m.meta.extra["app_name"].as_string();
                _code = m.meta.extra["code"].as_string();

                //unpack data
                _data.load(get_data_path(app_dir));
                u::dict data = u::decode<u::dict>(m.data);
                _data.import_from(data);

                if(_meta.id.empty()) 
                    throw std::runtime_error("received app `" + _meta.name + "' with empty id");
            }

            app::~app()
            {
                if(!_is_tmp) return;
                CHECK(!_meta.path.empty());
                LOG << "cleaning up tmp app `" << _meta.name << "' (" << _meta.id << ") at `" << _meta.path << "'" << std::endl;
                u::delete_directory(_meta.path);
            }

            app::operator m::message() const
            {
                INVARIANT_FALSE(_meta.id.empty());

                m::message m;

                m.meta.type = APP_MESSAGE;
                m.meta.extra["app_id"] = _meta.id;
                m.meta.extra["app_name"] = _meta.name;
                m.meta.extra["code"] = _code;
                
                //pack data
                u::dict data;
                _data.export_to(data);
                m.data = u::encode(data);

                return m;
            }

            app::app(const app& o) : 
                _meta(o._meta),
                _code{o._code},
                _data{o.data()},
                _local_data(o._local_data), 
                _is_tmp{o._is_tmp}
            {
            }

            app& app::operator=(const app& o)
            {
                if(this == &o) return *this;
                _meta = o._meta;
                _code = o._code;
                _data = o._data;
                _is_tmp = o._is_tmp;
                return *this;
            }

            app app::clone() const
            {
                app c = *this;
                c._meta.id = u::uuid();
                c._is_tmp = false;
                return c;
            }

            const std::string& app::path() const
            {
                return _meta.path;
            }

            const std::string& app::name() const
            {
                return _meta.name;
            }

            const std::string& app::id() const
            {
                ENSURE_FALSE(_meta.id.empty());
                return _meta.id;
            }

            const std::string& app::code() const
            {
                return _code;
            }

            bool app::launched_local() const
            {
                return _launched_local;
            }

            void app::path(const std::string& v)
            {
                _meta.path = v;
            }

            void app::name(const std::string& v)
            {
                _meta.name = v;
            }

            void app::code(const std::string& v)
            {
                _code = v;
            }

            void app::launched_local(bool v)
            {
                _launched_local = v;
            }

            void app::set_tmp()
            {
                _is_tmp = true;
            }

            const u::disk_store& app::data() const
            {
                return _data;
            }

            u::disk_store& app::data()
            {
                return _data;
            }

            const u::disk_store& app::local_data() const
            {
                return _local_data;
            }

            u::disk_store& app::local_data()
            {
                return _local_data;
            }

            bool save_app(const std::string& dir, const app& a)
            {
                REQUIRE_FALSE(a.id().empty());
                REQUIRE_FALSE(a.name().empty());

                if(!bf::exists(dir)) return false;

                //write metadata
                u::dict d;
                d["id"] = a.id();
                d["name"] = a.name();

                u::save_to_file(get_metadata_file(dir), d);

                //write code
                std::ofstream c(get_code_file(dir).c_str());
                if(!c.good()) return false;

                const auto& code = a.code();
                c.write(code.c_str(), code.size());

                //create data directories if they don't exist
                auto data_path = get_data_path(dir);
                u::create_directory(data_path);

                //copy data if the destination is not the same as source
                if(a.path() != dir)
                {
                    //remove existing data
                    u::delete_directory(data_path);
                    u::create_directory(data_path);

                    //copy data
                    //not the most efficient way of doing this :/
                    u::dict dc;
                    a.data().export_to(dc);
                    u::disk_store cs(data_path);
                    cs.import_from(dc);
                }
                
                return true;
            }

            app_ptr load_app(u::disk_store& local, const std::string& dir)
            {
                if(!bf::exists(dir)) return nullptr;

                //read metadata
                app_metadata m;
                if(!load_app_metadata(dir, m)) return nullptr;

                //read code
                std::ifstream c(get_code_file(dir).c_str());
                if(!c.good()) return nullptr;

                std::string code;
                std::string line;
                while(std::getline(c,line))
                {
                    code.append(line);
                    code.append("\n");
                } 

                app_ptr a{new app{local, m.id}};
                a->path(m.path);
                a->name(m.name);
                a->code(code);

                //create data directories if they don't exist
                u::create_directory(get_data_path(dir));

                //load app data
                a->data().load(get_data_path(dir));

                ENSURE(a);
                ENSURE_FALSE(a->path().empty());
                ENSURE_FALSE(a->id().empty());
                ENSURE_FALSE(a->name().empty());
                ENSURE_EQUAL(a->name(), m.name);
                ENSURE_EQUAL(a->id(), m.id);
                ENSURE_EQUAL(a->code(), code);
                return a; 
            }

            bool load_app_metadata(const std::string& dir, app_metadata& m)
            {
                REQUIRE_FALSE(dir.empty());

                util::dict d;
                if(!u::load_from_file(get_metadata_file(dir), d))
                    return false;

                m.path = dir;
                m.id = d["id"].as_string(); 
                m.name = d["name"].as_string(); 

                if(m.id.empty()) throw std::runtime_error("error loading app `" + dir + "', id is empty");
                if(m.name.empty()) throw std::runtime_error("error loading app `" + dir + "', name is empty");
                return true;
            }

            void export_app_as_message(const std::string& file, const app& a)
            {
                m::message m = a;
                auto b = u::compress(u::encode(m));
                u::save_to_file(file, b);
            }

            m::message import_app_as_message(const std::string& file)
            {
                u::bytes b;
                u::load_from_file(file, b);
                b = u::uncompress(b);
                m::message m;
                u::decode(b, m);
                return m;
            }
        }
    }
}
