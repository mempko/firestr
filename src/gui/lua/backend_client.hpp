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
#ifndef FIRESTR_GUI_API_BACKEND_CLIENT_H
#define FIRESTR_GUI_API_BACKEND_CLIENT_H

#include "gui/lua/api.hpp"
#include "gui/api/service.hpp"
#include "service/service.hpp"

namespace fire 
{
    namespace gui 
    {
        namespace lua 
        {
            struct backend_client : public api::backend, public service::service
            {
                public:
                    backend_client(lua_api_ptr, fire::message::mailbox_ptr);
                    
                public:
                    void run(const std::string& code);

                public:
                    virtual void button_clicked(api::ref_id);
                    virtual void dropdown_selected(api::ref_id, int item);
                    virtual void edit_edited(api::ref_id);
                    virtual void edit_finished(api::ref_id);
                    virtual void text_edit_edited(api::ref_id);
                    virtual void timer_triggered(api::ref_id);
                    virtual void got_sound(api::ref_id, const util::bytes&);
                    virtual void draw_mouse_pressed(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_released(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_dragged(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_moved(api::ref_id, int x, int y);

                    virtual void reset();

                private:
                    void init_handlers();
                    void received_script_message(const fire::message::message& m);
                    void received_event_message(const fire::message::message& m);

                private:
                    lua_api_ptr _api;
            };

            using backend_client_ptr = std::shared_ptr<backend_client>;
        }
    }
}

#endif

