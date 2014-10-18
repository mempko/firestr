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

#include "gui/lua/widgets.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            std::string button_ref::get_text() const
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->button_get_text(id);
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->button_set_text(id, t);
            }

            void button_ref::set_image(const image_ref& i)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->button_set_image(id, i.id);
            }

            void button_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                rp->second.callback = c;
                callback = c;
            }  

            void button_ref::handle(const std::string& t, const u::value& v)
            {
                INVARIANT(api);
                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;
                if(rp->second.callback.empty()) return;
                api->run(rp->second.callback);
            }

            std::string label_ref::get_text() const
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->label_get_text(id);
            }

            void label_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->label_set_text(id, t);
            }

            std::string edit_ref::get_text() const
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->edit_get_text(id);
            }

            void edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                api->front->edit_set_text(id, t);
            }

            void edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);
                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            void edit_ref::set_finished_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.finished_callback = c;
                finished_callback = c;
            }

            void edit_ref::handle(const std::string& t, const u::value& v)
            try
            {
                INVARIANT(api);
                INVARIANT(api->state);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                std::string callback;
                if(t == "e") callback = rp->second.edited_callback;
                else if( t == "f") callback = rp->second.finished_callback;

                if(callback.empty()) return;

                rp->second.can_callback = false;
                api->state->call(callback, v.as_string());
                rp->second.can_callback = true;
            }
            catch(...)
            {
                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;
                rp->second.can_callback = true;
                throw;
            }

            std::string text_edit_ref::get_text() const
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->text_edit_get_text(id);
            }

            void text_edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->text_edit_set_text(id, t);
            }

            void text_edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            void text_edit_ref::handle(const std::string& t, const u::value& v)
            try
            {
                INVARIANT(api);
                INVARIANT(api->state);
                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                if(rp->second.edited_callback.empty()) return;
                rp->second.can_callback = false;
                api->state->call(rp->second.edited_callback, v.as_string());
                rp->second.can_callback = true;
            }
            catch(...)
            {
                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;
                rp->second.can_callback = true;
                throw;
            }

            void list_ref::add(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id == 0);
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->list_add(id, wr.id);
            }

            void list_ref::remove(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id == 0);
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->list_remove(id, wr.id);
            }

            size_t list_ref::size() const
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->list_size(id);
            }

            void list_ref::clear()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->list_clear(id);
            }

            void grid_ref::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->grid_place(id, wr.id, r, c);
            }

            void grid_ref::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->grid_place_across(id, wr.id, r, c, row_span, col_span);
            }

            void draw_ref::set_mouse_released_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_released_callback = c;
                mouse_released_callback = c;
            }  

            void draw_ref::set_mouse_pressed_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_pressed_callback = c;
                mouse_pressed_callback = c;
            }  

            void draw_ref::set_mouse_moved_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_moved_callback = c;
                mouse_moved_callback = c;
            }  

            void draw_ref::set_mouse_dragged_callback(const std::string& c)
            {
                INVARIANT(api);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_dragged_callback = c;
                mouse_dragged_callback = c;
            }  

            void draw_ref::set_pen(pen_ref p)
            {
                INVARIANT(api);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.pen = p;
                pen = p;
            }

            void draw_ref::mouse_pressed(int button, int x, int y)
            try
            {
                INVARIANT(api);
                if(mouse_pressed_callback.empty()) return;

                api->state->call(mouse_pressed_callback, button, x, y);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in mouse_pressed: " << e.what();
                api->report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                api->report_error("error in mouse_pressed: unknown");
            }

            void draw_ref::mouse_released(int button, int x, int y)
            try
            {
                INVARIANT(api);
                if(mouse_released_callback.empty()) return;

                api->state->call(mouse_released_callback, button, x, y);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in mouse_released: " << e.what();
                api->report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                api->report_error("error in mouse_released: unknown");
            }

            void draw_ref::mouse_moved(int x, int y)
            try
            {
                INVARIANT(api);
                if(mouse_moved_callback.empty()) return;

                api->state->call(mouse_moved_callback, x, y);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in mouse_moved: " << e.what();
                api->report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                api->report_error("error in mouse_moved: unknown");
            }

            void draw_ref::mouse_dragged(int button, int x, int y)
            try
            {
                INVARIANT(api);
                if(mouse_dragged_callback.empty()) return;

                api->state->call(mouse_dragged_callback, button, x, y);
            }
            catch(SLB::CallException& e)
            {
                std::stringstream s;
                s << "error in mouse_dragged: " << e.what();
                api->report_error(s.str(), e.errorLine);
            }
            catch(...)
            {
                api->report_error("error in mouse_dragged: unknown");
            }


            void draw_ref::handle(const std::string& t, const u::value& v)
            {
                INVARIANT(api);
                u::dict d = v.as_dict();
                if(t == "p") 
                    mouse_pressed(d["b"].as_int(), d["x"].as_int(), d["y"].as_int());
                else if(t == "r") 
                    mouse_released(d["b"].as_int(), d["x"].as_int(), d["y"].as_int());
                else if(t == "d") 
                    mouse_dragged(d["b"].as_int(), d["x"].as_int(), d["y"].as_int());
                else if( t == "m") 
                    mouse_moved(d["x"].as_int(), d["y"].as_int());
            }

            void pen_ref::set_width(int w)
            {
                INVARIANT(api);
                INVARIANT(api->front);
                api->front->pen_set_width(id, w);
            }

            void draw_ref::line(double x1, double y1, double x2, double y2)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->draw_line(id, pen.id, x1, y1, x2, y2);
            }

            void draw_ref::circle(double x, double y, double r)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->draw_circle(id, pen.id, x, y, r);
            }

            void draw_ref::image(const image_ref& i, double x, double y, double w, double h)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                if(!i.good()) return;

                api->front->draw_image(id, i.id, x, y, w, h);
            }

            void draw_ref::clear()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->draw_clear(id);
            }

            bool timer_ref::running()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                return api->front->timer_running(id);
            }

            void timer_ref::stop()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->timer_stop(id);
            }

            void timer_ref::start()
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->timer_start(id);
            }

            void timer_ref::set_interval(int msec)
            {
                INVARIANT(api);
                INVARIANT(api->front);

                api->front->timer_set_interval(id, msec);
            }

            void timer_ref::set_callback(const std::string& c)  
            {
                INVARIANT(api);

                auto rp = api->timer_refs.find(id);
                if(rp == api->timer_refs.end()) return;

                rp->second.callback = c;
                callback = c;
            }

            int image_ref::width() const
            {
                INVARIANT(api);
                INVARIANT(api->front);
                return api->front->image_width(id);
            }

            int image_ref::height() const
            {
                INVARIANT(api);
                INVARIANT(api->front);
                return api->front->image_height(id);
            }

            bool image_ref::good() const
            {
                return g;
            }
        }
    }
}
