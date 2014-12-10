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

#ifndef FIRESTR_GUI_QTW_CLIENT_H
#define FIRESTR_GUI_QTW_CLIENT_H

#include "gui/api/service.hpp"
#include "gui/qtw/audio.hpp"
#include "gui/qtw/frontend.hpp"
#include "gui/list.hpp"
#include "conversation/conversation_service.hpp"

#include <QThread>
#include <thread>
#include <future>

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            using string_promise_ptr = std::shared_ptr<std::promise<std::string>>;
            using bool_promise_ptr = std::shared_ptr<std::promise<bool>>;
            using int_promise_ptr = std::shared_ptr<std::promise<int>>;
            using size_t_promise_ptr = std::shared_ptr<std::promise<size_t>>;
            using file_data_promise_ptr = std::shared_ptr<std::promise<api::file_data>>;
            using bin_file_data_promise_ptr = std::shared_ptr<std::promise<api::bin_file_data>>;

            class qt_frontend_client : public QObject, public api::frontend
            {
                Q_OBJECT

                public:
                    qt_frontend_client(qt_frontend_ptr);
                    ~qt_frontend_client();

                public:
                    void set_backend(api::backend*);
                    void stop();

                public:
                    //all widgets
                    virtual void place(api::ref_id, int r, int c);
                    virtual void place_across(api::ref_id id, int r, int c, int row_span, int col_span);
                    virtual void widget_enable(api::ref_id, bool);
                    virtual bool is_widget_enabled(api::ref_id);
                    virtual void widget_visible(api::ref_id, bool);
                    virtual bool is_widget_visible(api::ref_id);

                    //grid
                    virtual void add_grid(api::ref_id);
                    virtual void grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c);
                    virtual void grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span);

                    //button
                    virtual void add_button(api::ref_id, const std::string&);
                    virtual std::string button_get_text(api::ref_id);
                    virtual void button_set_text(api::ref_id, const std::string&);
                    virtual void button_set_image(api::ref_id id, api::ref_id image_id);

                    //label
                    virtual void add_label(api::ref_id, const std::string& text);
                    virtual std::string label_get_text(api::ref_id);
                    virtual void label_set_text(api::ref_id, const std::string& text);

                    //edit
                    virtual void add_edit(api::ref_id id, const std::string& text);
                    virtual std::string edit_get_text(api::ref_id);
                    virtual void edit_set_text(api::ref_id, const std::string& text);

                    //text edit
                    virtual void add_text_edit(api::ref_id id, const std::string& text);
                    virtual std::string text_edit_get_text(api::ref_id);
                    virtual void text_edit_set_text(api::ref_id, const std::string& text);

                    //list
                    virtual void add_list(api::ref_id id);
                    virtual void list_add(api::ref_id list_id, api::ref_id widget_id);
                    virtual void list_remove(api::ref_id list_id, api::ref_id widget_id);
                    virtual size_t list_size(api::ref_id);
                    virtual void list_clear(api::ref_id);

                    //pen
                    virtual void add_pen(api::ref_id id, const std::string& color, int width);
                    virtual void pen_set_width(api::ref_id id, int width);

                    //draw
                    virtual void add_draw(api::ref_id id, int width, int height);
                    virtual void draw_line(api::ref_id id, api::ref_id line, api::ref_id pen_id, double x1, double y1, double x2, double y2);
                    virtual void draw_circle(api::ref_id id, api::ref_id circle, api::ref_id pen_id, double x, double y, double r);
                    virtual void draw_image(api::ref_id id, api::ref_id image, api::ref_id image_id, double x, double y, double w, double h);
                    virtual void draw_clear(api::ref_id id);

                    //draw_line
                    virtual void draw_line_set(api::ref_id id, api::ref_id line, double x1, double y1, double x2, double y2);
                    virtual void draw_line_set_pen(api::ref_id id, api::ref_id line, api::ref_id pen_id);

                    //draw_circle
                    virtual void draw_circle_set(api::ref_id id, api::ref_id circle, double x, double y, double r);
                    virtual void draw_circle_set_pen(api::ref_id id, api::ref_id circle, api::ref_id pen_id);

                    //draw_image
                    virtual void draw_image_set(api::ref_id id, api::ref_id image, double x, double y, double w, double h);



                    //timer
                    virtual void add_timer(api::ref_id id, int msec);
                    virtual bool timer_running(api::ref_id id);
                    virtual void timer_stop(api::ref_id id);
                    virtual void timer_start(api::ref_id id);
                    virtual void timer_set_interval(api::ref_id, int msec);

                    //image
                    virtual bool add_image(api::ref_id, const util::bytes& d);
                    virtual int image_width(api::ref_id);
                    virtual int image_height(api::ref_id);

                    //mic
                    virtual void add_mic(api::ref_id id, const std::string& codec);
                    virtual void mic_start(api::ref_id);
                    virtual void mic_stop(api::ref_id);

                    //speaker
                    virtual void add_speaker(api::ref_id, const std::string& codec);
                    virtual void speaker_mute(api::ref_id);
                    virtual void speaker_unmute(api::ref_id);
                    virtual void speaker_play(api::ref_id, const util::bytes&);

                    //file
                    virtual api::file_data open_file();
                    virtual api::bin_file_data open_bin_file();
                    virtual bool save_file(const std::string&, const std::string&);
                    virtual bool save_bin_file(const std::string&, const util::bytes&);

                    //debug
                    virtual void print(const std::string&);

                    //overall gui
                    virtual void height(int h);
                    virtual void grow();
                    virtual bool visible();
                    virtual void alert();

                    //errors
                    virtual void report_error(const std::string& e);

                    virtual void reset();

                signals:
                    //all widgets
                    void got_place(api::ref_id, int r, int c);
                    void got_place_across(api::ref_id id, int r, int c, int row_span, int col_span);
                    void got_widget_enable(api::ref_id, bool);
                    bool got_is_widget_enabled(api::ref_id, bool_promise_ptr);
                    void got_widget_visible(api::ref_id, bool);
                    bool got_is_widget_visible(api::ref_id, bool_promise_ptr);

                    //grid
                    void got_add_grid(api::ref_id);
                    void got_grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c);
                    void got_grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span);

                    //button
                    void got_add_button(api::ref_id, const std::string&);
                    void got_button_get_text(api::ref_id, string_promise_ptr);
                    void got_button_set_text(api::ref_id, const std::string&);
                    void got_button_set_image(api::ref_id id, api::ref_id image_id);

                    //label
                    void got_add_label(api::ref_id, const std::string& text);
                    void got_label_get_text(api::ref_id, string_promise_ptr);
                    void got_label_set_text(api::ref_id, const std::string& text);

                    //edit
                    void got_add_edit(api::ref_id id, const std::string& text);
                    void got_edit_get_text(api::ref_id, string_promise_ptr);
                    void got_edit_set_text(api::ref_id, const std::string& text);

                    //text edit
                    void got_add_text_edit(api::ref_id id, const std::string& text);
                    void got_text_edit_get_text(api::ref_id, string_promise_ptr);
                    void got_text_edit_set_text(api::ref_id, const std::string& text);

                    //list
                    void got_add_list(api::ref_id id);
                    void got_list_add(api::ref_id list_id, api::ref_id widget_id);
                    void got_list_remove(api::ref_id list_id, api::ref_id widget_id);
                    void got_list_clear(api::ref_id);
                    void got_list_size(api::ref_id, size_t_promise_ptr);

                    //pen
                    void got_add_pen(api::ref_id id, const std::string& color, int width);
                    void got_pen_set_width(api::ref_id id, int width);

                    //draw
                    void got_add_draw(api::ref_id id, int width, int height);
                    void got_draw_line(api::ref_id id, api::ref_id line_id, api::ref_id pen_id, double x1, double y1, double x2, double y2);
                    void got_draw_circle(api::ref_id id, api::ref_id circle_id, api::ref_id pen_id, double x, double y, double r);
                    void got_draw_image(api::ref_id id, api::ref_id image_ref_id, api::ref_id image_id, double x, double y, double w, double h);
                    void got_draw_clear(api::ref_id id);

                    //draw_line
                    void got_draw_line_set(api::ref_id id, api::ref_id line, double x1, double y1, double x2, double y2);
                    void got_draw_line_set_pen(api::ref_id id, api::ref_id line, api::ref_id pen_id);

                    //draw_circle
                    void got_draw_circle_set(api::ref_id id, api::ref_id circle, double x, double y, double r);
                    void got_draw_circle_set_pen(api::ref_id id, api::ref_id circle, api::ref_id pen_id);

                    //draw_image
                    void got_draw_image_set(api::ref_id id, api::ref_id image, double x, double y, double w, double h);

                    //timer
                    void got_add_timer(api::ref_id id, int msec);
                    void got_timer_stop(api::ref_id id);
                    void got_timer_start(api::ref_id id);
                    void got_timer_set_interval(api::ref_id, int msec);
                    void got_timer_running(api::ref_id, bool_promise_ptr);

                    //image
                    void got_add_image(api::ref_id, const util::bytes& d, bool_promise_ptr);
                    void got_image_width(api::ref_id, int_promise_ptr);
                    void got_image_height(api::ref_id, int_promise_ptr);

                    //mic
                    void got_add_mic(api::ref_id id, const std::string& codec);
                    void got_mic_start(api::ref_id);
                    void got_mic_stop(api::ref_id);

                    //speaker
                    void got_add_speaker(api::ref_id, const std::string& codec);
                    void got_speaker_mute(api::ref_id);
                    void got_speaker_unmute(api::ref_id);
                    void got_speaker_play(api::ref_id, const util::bytes&);

                    //file
                    void got_save_file(const std::string&, const std::string&, bool_promise_ptr);
                    void got_save_bin_file(const std::string&, const util::bytes&, bool_promise_ptr);
                    void got_open_file(file_data_promise_ptr);
                    void got_open_bin_file(bin_file_data_promise_ptr);

                    //debug
                    void got_print(const std::string&);

                    //overall gui
                    void got_height(int h);
                    void got_grow();

                    //errors
                    void got_report_error(const std::string& e);

                    void got_reset();
                    void got_visible(bool_promise_ptr);
                    void got_alert();

                public slots:
                    void do_place(api::ref_id, int r, int c);
                    void do_place_across(api::ref_id id, int r, int c, int row_span, int col_span);
                    void do_widget_enable(api::ref_id, bool);
                    void do_is_widget_enabled(api::ref_id, bool_promise_ptr);
                    void do_widget_visible(api::ref_id, bool);
                    void do_is_widget_visible(api::ref_id, bool_promise_ptr);

                    //grid
                    void do_add_grid(api::ref_id);
                    void do_grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c);
                    void do_grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span);

                    //button
                    void do_add_button(api::ref_id, const std::string&);
                    void do_button_get_text(api::ref_id, string_promise_ptr);
                    void do_button_set_text(api::ref_id, const std::string&);
                    void do_button_set_image(api::ref_id id, api::ref_id image_id);

                    //label
                    void do_add_label(api::ref_id, const std::string& text);
                    void do_label_get_text(api::ref_id, string_promise_ptr);
                    void do_label_set_text(api::ref_id, const std::string& text);

                    //edit
                    void do_add_edit(api::ref_id id, const std::string& text);
                    void do_edit_get_text(api::ref_id, string_promise_ptr);
                    void do_edit_set_text(api::ref_id, const std::string& text);

                    //text edit
                    void do_add_text_edit(api::ref_id id, const std::string& text);
                    void do_text_edit_get_text(api::ref_id, string_promise_ptr);
                    void do_text_edit_set_text(api::ref_id, const std::string& text);

                    //list
                    void do_add_list(api::ref_id id);
                    void do_list_add(api::ref_id list_id, api::ref_id widget_id);
                    void do_list_remove(api::ref_id list_id, api::ref_id widget_id);
                    void do_list_clear(api::ref_id);
                    void do_list_size(api::ref_id, size_t_promise_ptr);

                    //pen
                    void do_add_pen(api::ref_id id, const std::string& color, int width);
                    void do_pen_set_width(api::ref_id id, int width);

                    //draw
                    void do_add_draw(api::ref_id id, int width, int height);
                    void do_draw_line(api::ref_id id, api::ref_id line_id, api::ref_id pen_id, double x1, double y1, double x2, double y2);
                    void do_draw_circle(api::ref_id id, api::ref_id circle_id, api::ref_id pen_id, double x, double y, double r);
                    void do_draw_image(api::ref_id id, api::ref_id image_ref_id, api::ref_id image_id, double x, double y, double w, double h);
                    void do_draw_clear(api::ref_id id);

                    //draw_line
                    void do_draw_line_set(api::ref_id id, api::ref_id line, double x1, double y1, double x2, double y2);
                    void do_draw_line_set_pen(api::ref_id id, api::ref_id line, api::ref_id pen_id);

                    //draw_circle
                    void do_draw_circle_set(api::ref_id id, api::ref_id circle, double x, double y, double r);
                    void do_draw_circle_set_pen(api::ref_id id, api::ref_id circle, api::ref_id pen_id);

                    //draw_image
                    void do_draw_image_set(api::ref_id id, api::ref_id image, double x, double y, double w, double h);

                    //timer
                    void do_add_timer(api::ref_id id, int msec);
                    void do_timer_stop(api::ref_id id);
                    void do_timer_start(api::ref_id id);
                    void do_timer_set_interval(api::ref_id, int msec);
                    void do_timer_running(api::ref_id, bool_promise_ptr);

                    //image
                    void do_add_image(api::ref_id, const util::bytes& d, bool_promise_ptr);
                    void do_image_width(api::ref_id, int_promise_ptr);
                    void do_image_height(api::ref_id, int_promise_ptr);

                    //mic
                    void do_add_mic(api::ref_id id, const std::string& codec);
                    void do_mic_start(api::ref_id);
                    void do_mic_stop(api::ref_id);

                    //speaker
                    void do_add_speaker(api::ref_id, const std::string& codec);
                    void do_speaker_mute(api::ref_id);
                    void do_speaker_unmute(api::ref_id);
                    void do_speaker_play(api::ref_id, const util::bytes&);

                    //file
                    void do_save_file(const std::string&, const std::string&, bool_promise_ptr);
                    void do_save_bin_file(const std::string&, const util::bytes&, bool_promise_ptr);
                    void do_open_file(file_data_promise_ptr);
                    void do_open_bin_file(bin_file_data_promise_ptr);

                    //debug
                    void do_print(const std::string&);

                    //overall gui
                    void do_height(int h);
                    void do_grow();
                    void do_visible(bool_promise_ptr);
                    void do_alert();

                    //errors
                    void do_report_error(const std::string& e);

                    void do_reset();

                private:
                    qt_frontend_ptr _f;
                    bool _done = false;
            };

            using qt_frontend_client_ptr = std::shared_ptr<qt_frontend_client>;
        }
    }
}

#endif


