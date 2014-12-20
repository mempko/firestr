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
#ifndef FIRESTR_GUI_API_SERVICE_H
#define FIRESTR_GUI_API_SERVICE_H

#include "util/bytes.hpp"

#include <string>

namespace fire 
{
    namespace gui 
    {
        namespace api 
        {
            using ref_id = std::size_t;

            template<class T>
                struct gen_file_data
                {
                    std::string name;
                    T data;
                    bool good = false;
                };

            using file_data = gen_file_data<std::string>;
            using bin_file_data = gen_file_data<util::bytes>;

            struct frontend
            {
                //all widgets
                virtual void place(ref_id, int r, int c) = 0;
                virtual void place_across(ref_id id, int r, int c, int row_span, int col_span) = 0;
                virtual void widget_enable(ref_id, bool) = 0;
                virtual bool is_widget_enabled(ref_id) = 0;
                virtual void widget_visible(ref_id, bool) = 0;
                virtual bool is_widget_visible(ref_id) = 0;
                virtual void widget_set_style(ref_id, const std::string&) = 0;

                //grid
                virtual void add_grid(ref_id) = 0;
                virtual void grid_place(ref_id grid_id, ref_id widget_id, int r, int c) = 0;
                virtual void grid_place_across(ref_id grid_id, ref_id widget_id, int r, int c, int row_span, int col_span) = 0;

                //button
                virtual void add_button(ref_id, const std::string&) = 0;
                virtual std::string button_get_text(ref_id) = 0;
                virtual void button_set_text(ref_id, const std::string&) = 0;
                virtual void button_set_image(ref_id, ref_id image_id) = 0;

                //make label
                virtual void add_label(ref_id, const std::string& text) = 0;
                virtual std::string label_get_text(ref_id) = 0;
                virtual void label_set_text(ref_id, const std::string& text) = 0;

                //edit
                virtual void add_edit(ref_id, const std::string& text) = 0;
                virtual std::string edit_get_text(ref_id) = 0;
                virtual void edit_set_text(ref_id, const std::string& text) = 0;

                //text edit
                virtual void add_text_edit(ref_id, const std::string& text) = 0;
                virtual std::string text_edit_get_text(ref_id) = 0;
                virtual void text_edit_set_text(ref_id, const std::string& text) = 0;

                //list
                virtual void add_list(ref_id) = 0;
                virtual void list_add(ref_id list_id, ref_id widget_id) = 0;
                virtual void list_remove(ref_id list_id, ref_id widget_id) = 0;
                virtual size_t list_size(ref_id) = 0;
                virtual void list_clear(ref_id) = 0;

                //dropdown
                virtual void add_dropdown(ref_id) = 0;
                virtual size_t dropdown_size(ref_id) = 0;
                virtual void dropdown_add_item(ref_id, const std::string&) = 0;
                virtual std::string dropdown_get_item(ref_id, int index) = 0;
                virtual int dropdown_get_selected(ref_id) = 0;
                virtual void dropdown_select(ref_id, int) = 0;
                virtual void dropdown_clear(ref_id) = 0;

                //pen
                virtual void add_pen(ref_id, const std::string& color, int width) = 0;
                virtual void pen_set_width(ref_id, int width) = 0;

                //draw
                virtual void add_draw(ref_id, int width, int height) = 0;
                virtual void draw_line(ref_id, ref_id line, ref_id pen_id, double x1, double y1, double x2, double y2) = 0;
                virtual void draw_circle(ref_id, ref_id circle, ref_id pen_id, double x, double y, double r) = 0;
                virtual void draw_image(ref_id, ref_id image, ref_id image_id, double x, double y, double w, double h) = 0;
                virtual void draw_clear(ref_id id) = 0;

                //draw_line
                virtual void draw_line_set(ref_id id, ref_id line, double x1, double y1, double x2, double y2) = 0;
                virtual void draw_line_set_pen(ref_id id, ref_id line, ref_id pen_id) = 0;

                //draw_circle
                virtual void draw_circle_set(ref_id id, ref_id circle, double x, double y, double r) = 0;
                virtual void draw_circle_set_pen(ref_id id, ref_id circle, ref_id pen_id) = 0;

                //draw_image
                virtual void draw_image_set(ref_id id, ref_id image, double x, double y, double w, double h) = 0;

                //timer
                virtual void add_timer(ref_id, int msec) = 0;
                virtual bool timer_running(ref_id id) = 0;
                virtual void timer_stop(ref_id id) = 0;
                virtual void timer_start(ref_id id) = 0;
                virtual void timer_set_interval(ref_id, int msec) = 0;

                //image
                virtual bool add_image(ref_id, const util::bytes& d) = 0;
                virtual int image_width(ref_id) = 0;
                virtual int image_height(ref_id) = 0;

                //mic
                virtual void add_mic(ref_id, const std::string& codec) = 0;
                virtual void mic_start(ref_id) = 0;
                virtual void mic_stop(ref_id) = 0;

                //speaker
                virtual void add_speaker(ref_id, const std::string& codec) = 0;
                virtual void speaker_mute(ref_id) = 0;
                virtual void speaker_unmute(ref_id) = 0;
                virtual void speaker_play(ref_id, const util::bytes&) = 0;

                //file
                virtual file_data open_file() = 0;
                virtual bin_file_data open_bin_file() = 0;
                virtual bool save_file(const std::string& suggested_name, const std::string& data) = 0;
                virtual bool save_bin_file(const std::string& suggested_name, const util::bytes& data) = 0;

                //debug
                virtual void print(const std::string&) = 0;

                //overall gui
                virtual void height(int h) = 0;
                virtual bool visible() = 0;
                virtual void grow() = 0;
                virtual void alert() = 0;

                //errors
                virtual void report_error(const std::string& e) = 0;

                //control
                virtual void reset() = 0;
            };

            struct backend
            {
                virtual void button_clicked(ref_id) = 0;
                virtual void dropdown_selected(ref_id, int item) = 0;
                virtual void edit_edited(ref_id) = 0;
                virtual void edit_finished(ref_id) = 0;
                virtual void text_edit_edited(ref_id) = 0;
                virtual void timer_triggered(ref_id) = 0;
                virtual void got_sound(ref_id, const util::bytes&) = 0;
                virtual void draw_mouse_pressed(ref_id, int button, int x, int y) = 0;
                virtual void draw_mouse_released(ref_id, int button, int x, int y) = 0;
                virtual void draw_mouse_dragged(ref_id, int button, int x, int y) = 0;
                virtual void draw_mouse_moved(ref_id, int x, int y) = 0;

                virtual void reset() = 0;
            };

        }
    }
}

#endif

