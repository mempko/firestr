/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
            struct button_ref : public widget_ref
            {
                std::string callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_callback() const { return callback;}
                void set_callback(const std::string&);  
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
            };
            using edit_ref_map = std::unordered_map<int, edit_ref>;

            struct text_edit_ref : public widget_ref
            {
                std::string edited_callback;

                std::string get_text() const; 
                void set_text(const std::string&);

                const std::string& get_edited_callback() const { return edited_callback;}
                void set_edited_callback(const std::string&);  
            };
            using text_edit_ref_map = std::unordered_map<int, text_edit_ref>;

            struct list_ref : public widget_ref
            {
                void add(const widget_ref& r);
                void clear();
            };
            using list_ref_map = std::unordered_map<int, list_ref>;

            //decided to call layouts canvases
            struct canvas_ref : public basic_ref
            {
                void place(const widget_ref& w, int r, int c);
                void place_across(const widget_ref& w, int r, int c, int row_span, int col_span);
            };

            using canvas_ref_map = std::unordered_map<int, canvas_ref>;

            class draw_view;
            struct draw_ref : public widget_ref
            {
                std::string mouse_released_callback;
                std::string mouse_pressed_callback;
                std::string mouse_moved_callback;
                std::string mouse_dragged_callback;

                void clear();
                void line(double x1, double y1, double x2, double y2);

                const std::string& get_mouse_released_callback() const { return mouse_released_callback;}
                const std::string& get_mouse_pressed_callback() const { return mouse_pressed_callback;}
                const std::string& get_mouse_moved_callback() const { return mouse_moved_callback;}
                const std::string& get_mouse_dragged_callback() const { return mouse_dragged_callback;}
                void set_mouse_released_callback(const std::string&);  
                void set_mouse_pressed_callback(const std::string&);  
                void set_mouse_moved_callback(const std::string&);  
                void set_mouse_dragged_callback(const std::string&);  
                void set_pen(QPen);
                QPen get_pen() { return pen;}

                void mouse_pressed(int button, int x, int y);
                void mouse_released(int button, int x, int y);
                void mouse_moved(int x, int y);
                void mouse_dragged(int button, int x, int y);

                draw_view* get_view();
                QPen pen;
            };
            using draw_ref_map = std::unordered_map<int, draw_ref>;

            class draw_view : public QGraphicsView
            {
                Q_OBJECT
                public:
                    draw_view(draw_ref, int width, int height);

                protected:
                    void mousePressEvent(QMouseEvent*);
                    void mouseReleaseEvent(QMouseEvent*);
                    void mouseMoveEvent(QMouseEvent*);

                private:
                    draw_ref _ref;
                    int _button;
            };
        }
    }
}

#endif
