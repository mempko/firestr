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

#include "gui/app/app.hpp"
#include "util/mencode.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/filesystem.hpp"

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
            }

            app::app(u::disk_store& s, const std::string& id) : _local_data(s)
            {
                REQUIRE_FALSE(id.empty());
                _meta.id = id;
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app(u::disk_store& s) : _local_data(s)
            {
                _meta.id = u::uuid();
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app(u::disk_store& s, const m::message& m) : _local_data(s)
            {
                REQUIRE_EQUAL(m.meta.type, APP_MESSAGE);

                _meta.id = m.meta.extra["app_id"].as_string();
                _meta.name = m.meta.extra["app_name"].as_string();
                _code = u::to_str(m.data);

                if(_meta.id.empty()) 
                    throw std::runtime_error("received app `" + _meta.name + "' with empty id");
            }

            app::operator m::message()
            {
                INVARIANT_FALSE(_meta.id.empty());

                m::message m;

                m.meta.type = APP_MESSAGE;
                m.meta.extra["app_id"] = _meta.id;
                m.meta.extra["app_name"] = _meta.name;
                m.data = u::to_bytes(_code);

                return m;
            }

            app::app(const app& o) : 
                _meta(o._meta),
                _code(o._code),
                _local_data(o._local_data), 
                _data(o.data())
            {
            }

            app& app::operator=(const app& o)
            {
                if(this == &o) return *this;
                _meta = o._meta;
                _code = o._code;
                _data = o._data;
                return *this;
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
                u::create_directory(get_data_path(dir));
                
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
        }
    }
}
