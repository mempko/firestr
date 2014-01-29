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

#include "util/disk_store.hpp"

#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/uuid.hpp"
#include "util/filesystem.hpp"

#include <stdexcept>

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

namespace fire
{
    namespace util
    {
        namespace 
        {
            const std::string INDEX = "index";
        }

        std::string get_index_file(const std::string& dir)
        {
            bf::path p = dir;

            p /= INDEX;
            return p.string();
        }

        std::string get_value_file(const std::string& dir, const std::string& id)
        {
            bf::path p = dir;

            p /= id;
            return p.string();
        }

        disk_store::disk_store() 
        {
            _mutex = std::make_shared<std::mutex>();
            _index = std::make_shared<dict>();
            ENSURE(_path.empty());
            ENSURE(_mutex);
            ENSURE(_index);
        }

        disk_store::disk_store(const std::string& path) 
        {
            _mutex = std::make_shared<std::mutex>();
            _index = std::make_shared<dict>();

            REQUIRE_FALSE(path.empty());
            CHECK(_path.empty());

            load(path);

            ENSURE_FALSE(_path.empty());
            ENSURE(_mutex);
            ENSURE(_index);
        }

        disk_store::disk_store(const disk_store& o) 
        {
            REQUIRE(o._mutex);
            REQUIRE(o._index);
            mutex_scoped_lock l(*o._mutex);

            _path = o._path;
            _index = o._index;
            _mutex = o._mutex;

            ENSURE(_mutex);
            ENSURE(_index);
        }

        disk_store& disk_store::operator=(const disk_store& o)
        {
            INVARIANT(_mutex);
            INVARIANT(_index);
            REQUIRE(o._mutex);
            REQUIRE(o._index);

            if(this == &o) return *this;
            if(_mutex == o._mutex) return *this;

            CHECK(_index != _index);
            mutex_scoped_lock l(*o._mutex);
            {
                mutex_scoped_lock l(*_mutex);
                _path = o._path;
                _index = o._index;
            }
            _mutex = o._mutex;

            ENSURE(_mutex);
            ENSURE(_index);
        }

        void disk_store::load(const std::string& path) 
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);
            REQUIRE_FALSE(path.empty());

            if(!bf::exists(path)) 
                throw std::runtime_error{"path `" + path + "' does not exist."};

            LOG << "loading store `" << path << "'" << std::endl;
            _path = path;
            load_from_file(get_index_file(path), *_index);

            ENSURE_FALSE(_path.empty());
        }

        bool disk_store::loaded() const
        {
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);
            return !_path.empty();
        }

        value disk_store::get(const std::string& key) const
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);
            INVARIANT_FALSE(_path.empty());

            value v;
            get_value(key, v);
            return v;
        }

        void disk_store::get_value(const std::string& key, value& v) const
        {
            INVARIANT(_index);

            auto id = (*_index)[key].as_string();
            load_from_file(get_value_file(_path, id), v);
        }

        bool disk_store::has(const std::string& key) const
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);
            return _index->has(key);
        }
        void disk_store::set(const std::string& key, const value& v)
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);

            set_intern(key, v);
            save_index();
        }

        void disk_store::set_intern(const std::string& key, const value& v)
        {
            INVARIANT(_index);

            std::string id;
            if(_index->has(key)) id = (*_index)[key].as_string();
            else 
            {
                id = uuid();
                (*_index)[key] = id;
            }

            CHECK_FALSE(id.empty());
            save_to_file(get_value_file(_path, id), v);
        }

        void disk_store::save_index()
        {
            INVARIANT_FALSE(_path.empty());
            save_to_file(get_index_file(_path), *_index);
        }

        void disk_store::import_from(const dict& d)
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);

            for(const auto& p : d)
                set_intern(p.first, p.second);

            save_index();
        }

        void disk_store::export_to(dict& d) const
        {
            INVARIANT(_index);
            INVARIANT(_mutex);
            mutex_scoped_lock l(*_mutex);

            for(const auto& p : *_index)
            {
                value v;
                get_value(p.first, v);
                d[p.first]  = v;
            }
        }
    }
}
