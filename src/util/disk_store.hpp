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
                void import_from(const dict&);
                void export_to(dict&) const;

            public:
                value get(const std::string& key) const;
                bool has(const std::string& key) const;
                void set(const std::string& key, const value&);
                bool remove(const std::string& key);

            public:
                using const_iterator = dict::const_iterator;
                const_iterator begin() const;
                const_iterator end() const;
                size_t size() const;

            private:
                void save_index();
                void set_intern(const std::string& key, const value&);
                void get_value(const std::string& key, value&) const;

            private:
                std::string _path;
                dict_ptr _index;
                mutable mutex_ptr _mutex;
        };

    }
}

#endif
