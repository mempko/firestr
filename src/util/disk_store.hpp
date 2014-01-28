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

#ifndef FIRESTR_UIL_DISK_STORE_H
#define FIRESTR_UIL_DISK_STORE_H

#include "util/mencode.hpp"
#include "util/thread.hpp"

namespace fire
{
    namespace util
    {
        class disk_store
        {
            public:
                disk_store();
                disk_store(const std::string& path);
                disk_store(const disk_store&);
                disk_store& operator=(const disk_store&);

            public:
                void load(const std::string& path);
                bool loaded() const;

            public:
                value get(const std::string& key) const;
                bool has(const std::string& key) const;
                void set(const std::string& key, const value&);

            private:
                void save_index();

            private:
                std::string _path;
                dict_ptr _index;
                mutable mutex_ptr _mutex;
        };

    }
}

#endif
