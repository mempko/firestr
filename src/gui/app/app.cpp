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
            }

            app::app(const std::string& id) 
            {
                REQUIRE_FALSE(id.empty());
                _meta.id = id;
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app() 
            {
                _meta.id = u::uuid();
                INVARIANT_FALSE(_meta.id.empty());
            }

            app::app(const m::message& m)
            {
                REQUIRE_EQUAL(m.meta.type, APP_MESSAGE);

                _meta.id = m.meta.extra["app_id"].as_string();
                _meta.name = m.meta.extra["app_name"].as_string();
                _code = u::to_str(m.data);

                if(_meta.id.empty()) 
                    throw std::runtime_error("recieved app `" + _meta.name + "' with empty id");
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

            void app::name(const std::string& v)
            {
                _meta.name = v;
            }

            void app::code(const std::string& v)
            {
                _code = v;
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

            bool save_app(const std::string& dir, const app& a)
            {
                REQUIRE_FALSE(a.id().empty());
                REQUIRE_FALSE(a.name().empty());

                if(!bf::exists(dir)) return false;

                //write metadata
                std::ofstream m(get_metadata_file(dir).c_str());
                if(!m.good()) return false;

                u::dict d;
                d["id"] = a.id();
                d["name"] = a.name();

                m << d;

                //write code
                std::ofstream c(get_code_file(dir).c_str());
                if(!c.good()) return false;

                const auto& code = a.code();
                c.write(code.c_str(), code.size());
                
                return true;
            }

            app_ptr load_app(const std::string& dir)
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

                app_ptr a{new app{m.id}};
                a->name(m.name);
                a->code(code);

                ENSURE(a);
                ENSURE_FALSE(a->id().empty());
                ENSURE_FALSE(a->name().empty());
                ENSURE_EQUAL(a->name(), m.name);
                ENSURE_EQUAL(a->id(), m.id);
                ENSURE_EQUAL(a->code(), code);
                return a; 
            }

            bool load_app_metadata(const std::string& dir, app_metadata& m)
            {
                std::ifstream in(get_metadata_file(dir).c_str());
                if(!in.good()) return false;

                util::dict d;
                in >> d;

                m.id = d["id"].as_string(); 
                m.name = d["name"].as_string(); 

                if(m.id.empty()) throw std::runtime_error("error loading app `" + dir + "', id is empty");
                if(m.name.empty()) throw std::runtime_error("error loading app `" + dir + "', name is empty");
                return true;
            }
        }
    }
}
