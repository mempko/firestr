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

#ifndef FIRESTR_GUI_APP_APP_H
#define FIRESTR_GUI_APP_APP_H

#include "message/message.hpp"
#include "util/disk_store.hpp"

#include <string>
#include <memory>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            struct app_metadata
            {
                std::string path;
                std::string id;
                std::string name;
            };

            class app
            {
                public:
                    app(util::disk_store& local);
                    app(util::disk_store& local, const std::string& id);
                    app(util::disk_store& local, 
                            const std::string& app_dir, 
                            const fire::message::message&);
                    ~app();
                    operator fire::message::message() const;

                public:
                    app(const app&);
                    app& operator=(const app&);
                    app clone() const;

                public:
                    const std::string& path() const;
                    const std::string& name() const;
                    const std::string& id() const;
                    const std::string& code() const;
                    bool launched_local() const;

                public:
                    void path(const std::string&);
                    void name(const std::string&);
                    void code(const std::string&);
                    void set_tmp();
                    void launched_local(bool);

                public:
                    //local storage that is transfered
                    const util::disk_store& data() const;
                    util::disk_store& data();

                    //data only stored on users computer
                    const util::disk_store& local_data() const;
                    util::disk_store& local_data();

                private:
                    app_metadata _meta;
                    std::string _code;
                    util::disk_store _data;
                    util::disk_store& _local_data;
                    bool _launched_local;
                    bool _is_tmp;
            };

            using app_ptr = std::shared_ptr<app>;
            using app_wptr = std::weak_ptr<app>;

            bool save_app(const std::string& dir, const app&);
            app_ptr load_app(util::disk_store& local, const std::string& dir);
            bool app_removed(const std::string& dir);
            bool tag_app_removed(const std::string& dir);
            bool load_app_metadata(const std::string& dir, app_metadata&);

            void export_app_as_message(const std::string& file, const app&);

            fire::message::message import_app_as_message(util::bytes compressed_app);
            fire::message::message import_app_as_message(const std::string& file);
        }
    }
}

#endif
