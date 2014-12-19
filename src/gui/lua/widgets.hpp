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

#ifndef FIRESTR_APP_LUA_WIDGETS_H
#define FIRESTR_APP_LUA_WIDGETS_H

#include "gui/lua/base.hpp"

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            struct pen_ref : public widget_ref 
            {
                void set_width(int);
            };

            struct image_ref;

            struct button_ref : public widget_ref
            {
                std::string callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                void set_image(const image_ref&);

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  
                void handle(const std::string& t,  const util::value& event);
            };
            using button_ref_map = std::unordered_map<int, button_ref>;

            struct label_ref : public widget_ref
            {
                std::string get_text() const; 
                void set_text(const std::string&);
            };
            using label_ref_map = std::unordered_map<int, label_ref>;

            struct edit_ref : public widget_ref
            {
                std::string edited_callback;
                std::string finished_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_edited_callback() const { return edited_callback;}
                void set_edited_callback(const std::string&);  

                const std::string& get_finished_callback() const { return finished_callback;}
                void set_finished_callback(const std::string&);  

                void handle(const std::string& t,  const util::value& event);
            };
            using edit_ref_map = std::unordered_map<int, edit_ref>;

            struct text_edit_ref : public widget_ref
            {
                std::string edited_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_edited_callback() const { return edited_callback;}
                void set_edited_callback(const std::string&);  

                void handle(const std::string& t,  const util::value& event);
            };
            using text_edit_ref_map = std::unordered_map<int, text_edit_ref>;

            class list_ref : public widget_ref
            {
                public:
                    void add(const widget_ref& r);
                    void remove(const widget_ref& r);
                    size_t size() const;
                    void clear();
            };
            using list_ref_map = std::unordered_map<int, list_ref>;

            struct dropdown_ref : public widget_ref
            {
                std::string callback;

                void add(const std::string&); 
                std::string get(int) const; 
                int selected() const; 
                size_t size() const;

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  
                void handle(const std::string& t,  const util::value& event);
            };
            using dropdown_ref_map = std::unordered_map<int, dropdown_ref>;

            //decided to call layouts canvases
            struct grid_ref : public widget_ref
            {
                void place(const widget_ref& w, int r, int c);
                void place_across(const widget_ref& w, int r, int c, int row_span, int col_span);
            };

            using grid_ref_map = std::unordered_map<int, grid_ref>;

            struct draw_line_ref : public basic_ref
            {
                int view_id;
                void set(double x1, double y1, double x2, double y2);
                void set_pen(pen_ref);
            };
            using draw_line_ref_map = std::unordered_map<int, draw_line_ref>;

            struct draw_circle_ref : public basic_ref
            {
                int view_id;
                void set(double x, double y, double r);
                void set_pen(pen_ref);
            };
            using draw_circle_ref_map = std::unordered_map<int, draw_circle_ref>;

            struct draw_image_ref : public basic_ref
            {
                int view_id;
                void set(double x, double y, double w, double h);
            };

            using draw_image_ref_map = std::unordered_map<int, draw_image_ref>;

            struct draw_ref : public widget_ref
            {
                std::string mouse_released_callback;
                std::string mouse_pressed_callback;
                std::string mouse_moved_callback;
                std::string mouse_dragged_callback;

                void clear();
                draw_line_ref line(double x1, double y1, double x2, double y2);
                draw_circle_ref circle(double x, double y, double r);
                draw_image_ref image(const image_ref& i, double x, double y, double w, double h);

                const std::string& get_mouse_released_callback() const { return mouse_released_callback;}
                const std::string& get_mouse_pressed_callback() const { return mouse_pressed_callback;}
                const std::string& get_mouse_moved_callback() const { return mouse_moved_callback;}
                const std::string& get_mouse_dragged_callback() const { return mouse_dragged_callback;}
                void set_mouse_released_callback(const std::string&);  
                void set_mouse_pressed_callback(const std::string&);  
                void set_mouse_moved_callback(const std::string&);  
                void set_mouse_dragged_callback(const std::string&);  
                void set_pen(pen_ref);
                pen_ref get_pen() { return pen;}

                void mouse_pressed(int button, int x, int y);
                void mouse_released(int button, int x, int y);
                void mouse_moved(int x, int y);
                void mouse_dragged(int button, int x, int y);

                void handle(const std::string& t,  const util::value& event);

                pen_ref pen;
                draw_line_ref_map lines;
                draw_circle_ref_map circles;
                draw_image_ref_map images;
            };
            using draw_ref_map = std::unordered_map<int, draw_ref>;

            struct timer_ref : public basic_ref
            {
                bool running();
                void stop();
                void start();
                void set_interval(int msec);
                std::string callback;

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  
            };
            using timer_ref_map = std::unordered_map<int, timer_ref>;

            struct image_ref : public widget_ref
            {
                int width() const;
                int height() const;
                bool good() const;
                int w, h;
                bool g;
            };

            using image_ref_map = std::unordered_map<int, image_ref>;

        }
    }
}

#endif
