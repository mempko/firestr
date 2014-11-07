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

#include "gui/qtw/frontend_client.hpp"

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            namespace
            {
                const std::chrono::seconds TIMEOUT{1};
            }

            template<class T, class D>
                T get_until(std::future<T>& f, const D& d)
                {
                    return f.wait_for(TIMEOUT) == std::future_status::timeout ? 
                        d : f.get();
                }

#define F_CON(x) connect(this, SIGNAL(got_##x), this, SLOT(do_##x))

            qt_frontend_client::qt_frontend_client(qt_frontend_ptr f) : _f{f}
            {
                REQUIRE(f);
                qRegisterMetaType<std::string>("std::string");
                qRegisterMetaType<util::bytes>("util::bytes");
                qRegisterMetaType<api::ref_id>("api::ref_id");
                qRegisterMetaType<string_promise_ptr>("string_promise_ptr");
                qRegisterMetaType<int_promise_ptr>("int_promise_ptr");
                qRegisterMetaType<bool_promise_ptr>("bool_promise_ptr");
                qRegisterMetaType<size_t_promise_ptr>("size_t_promise_ptr");
                qRegisterMetaType<file_data_promise_ptr>("file_data_promise_ptr");
                qRegisterMetaType<bin_file_data_promise_ptr>("bin_file_data_promise_ptr");

                F_CON(place(api::ref_id, int, int));
                F_CON(place_across(api::ref_id, int, int, int, int));
                F_CON(widget_enable(api::ref_id, bool));
                F_CON(is_widget_enabled(api::ref_id, bool_promise_ptr));

                //grid
                F_CON(add_grid(api::ref_id));
                F_CON(grid_place(api::ref_id, api::ref_id, int, int));
                F_CON(grid_place_across(api::ref_id, api::ref_id, int, int, int, int));

                //button
                F_CON(add_button(api::ref_id, const std::string&));
                F_CON(button_get_text(api::ref_id, string_promise_ptr));
                F_CON(button_set_text(api::ref_id, const std::string&));
                F_CON(button_set_image(api::ref_id, api::ref_id));

                //label
                F_CON(add_label(api::ref_id, const std::string&));
                F_CON(label_get_text(api::ref_id, string_promise_ptr));
                F_CON(label_set_text(api::ref_id, const std::string&));

                //edit
                F_CON(add_edit(api::ref_id, const std::string&));
                F_CON(edit_get_text(api::ref_id, string_promise_ptr));
                F_CON(edit_set_text(api::ref_id, const std::string&));

                //text edit
                F_CON(add_text_edit(api::ref_id, const std::string&));
                F_CON(text_edit_get_text(api::ref_id, string_promise_ptr));
                F_CON(text_edit_set_text(api::ref_id, const std::string&));

                //list
                F_CON(add_list(api::ref_id));
                F_CON(list_add(api::ref_id, api::ref_id));
                F_CON(list_remove(api::ref_id, api::ref_id));
                F_CON(list_size(api::ref_id, size_t_promise_ptr));
                F_CON(list_clear(api::ref_id));

                //pen
                F_CON(add_pen(api::ref_id, const std::string&, int));
                F_CON(pen_set_width(api::ref_id, int));

                //draw
                F_CON(add_draw(api::ref_id, int, int));
                F_CON(draw_line(api::ref_id, api::ref_id, double, double, double, double));
                F_CON(draw_circle(api::ref_id, api::ref_id, double, double, double));
                F_CON(draw_image(api::ref_id, api::ref_id, double, double, double, double));
                F_CON(draw_clear(api::ref_id));

                //timer
                F_CON(add_timer(api::ref_id, int));
                F_CON(timer_running(api::ref_id, bool_promise_ptr));
                F_CON(timer_stop(api::ref_id));
                F_CON(timer_start(api::ref_id));
                F_CON(timer_set_interval(api::ref_id, int));

                //image
                F_CON(add_image(api::ref_id, const util::bytes&, bool_promise_ptr));
                F_CON(image_width(api::ref_id, int_promise_ptr));
                F_CON(image_height(api::ref_id, int_promise_ptr));

                //mic
                F_CON(add_mic(api::ref_id, const std::string&));
                F_CON(mic_start(api::ref_id));
                F_CON(mic_stop(api::ref_id));

                //speaker
                F_CON(add_speaker(api::ref_id, const std::string&));
                F_CON(speaker_mute(api::ref_id));
                F_CON(speaker_unmute(api::ref_id));
                F_CON(speaker_play(api::ref_id, const util::bytes&));

                //file
                F_CON(open_file(file_data_promise_ptr));
                F_CON(open_bin_file(bin_file_data_promise_ptr));
                F_CON(save_file(const std::string&, const std::string&, bool_promise_ptr));
                F_CON(save_bin_file(const std::string&, const util::bytes&, bool_promise_ptr));

                //debug
                F_CON(print(const std::string&));

                //overall gui
                F_CON(height(int));
                F_CON(grow());
                F_CON(visible(bool_promise_ptr));
                F_CON(alert());

                //errors
                F_CON(report_error(const std::string&));

                F_CON(reset());

                ENSURE(_f);
            }

            qt_frontend_client::~qt_frontend_client()
            {
                if(!_done) stop();
            }

            void qt_frontend_client::stop()
            {
                _done = true;
            }

            void qt_frontend_client::set_backend(api::backend* b)
            {
                REQUIRE(b);
                INVARIANT(_f);
                _f->set_backend(b);
            }

            //all widgets
            void qt_frontend_client::place(api::ref_id id, int r, int c)
            {
                if(_done) return;
                emit got_place(id, r, c);
            }

            void qt_frontend_client::place_across(api::ref_id id, int r, int c, int row_span, int col_span)
            {
                if(_done) return;
                emit got_place_across(id, r, c, row_span, col_span);

            }

            void qt_frontend_client::widget_enable(api::ref_id id, bool b)
            {
                if(_done) return;
                emit got_widget_enable(id, b);
            }

            bool qt_frontend_client::is_widget_enabled(api::ref_id id)
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_is_widget_enabled(id, p);

                return get_until(f, false);
            }

            //grid
            void qt_frontend_client::add_grid(api::ref_id id)
            {
                if(_done) return;
                emit got_add_grid(id);
            }

            void qt_frontend_client::grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c)
            {
                if(_done) return;
                emit got_grid_place(grid_id, widget_id, r, c);
            }

            void qt_frontend_client::grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span)
            {
                if(_done) return;
                emit got_grid_place_across(grid_id, widget_id, r, c, row_span, col_span);
            }


            //button
            void qt_frontend_client::add_button(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_add_button(id, t);
            }

            std::string qt_frontend_client::button_get_text(api::ref_id id)
            {
                if(_done) return "";
                auto p = std::make_shared<std::promise<std::string>>();
                auto f = p->get_future();

                emit got_button_get_text(id, p);

                return get_until(f, "");
            }

            void qt_frontend_client::button_set_text(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_button_set_text(id, t);

            }

            void qt_frontend_client::button_set_image(api::ref_id id, api::ref_id image_id)
            {
                if(_done) return;
                emit got_button_set_image(id, image_id);
            }


            //label
            void qt_frontend_client::add_label(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_add_label(id, t);
            }

            std::string qt_frontend_client::label_get_text(api::ref_id id)
            {
                if(_done) return "";
                auto p = std::make_shared<std::promise<std::string>>();
                auto f = p->get_future();

                emit got_label_get_text(id, p);

                return get_until(f, "");
            }

            void qt_frontend_client::label_set_text(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_label_set_text(id, t);
            }


            //edit
            void qt_frontend_client::add_edit(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_add_edit(id, t);
            }

            std::string qt_frontend_client::edit_get_text(api::ref_id id)
            {
                if(_done) return "";
                auto p = std::make_shared<std::promise<std::string>>();
                auto f = p->get_future();

                emit got_edit_get_text(id, p);

                return get_until(f, "");
            }

            void qt_frontend_client::edit_set_text(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_edit_set_text(id, t);
            }

            //text edit
            void qt_frontend_client::add_text_edit(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_add_text_edit(id, t);
            }

            std::string qt_frontend_client::text_edit_get_text(api::ref_id id)
            {
                if(_done) return "";
                auto p = std::make_shared<std::promise<std::string>>();
                auto f = p->get_future();

                emit got_text_edit_get_text(id, p);

                return get_until(f, "");
            }

            void qt_frontend_client::text_edit_set_text(api::ref_id id, const std::string& t)
            {
                if(_done) return;
                emit got_text_edit_set_text(id, t);
            }


            //list
            void qt_frontend_client::add_list(api::ref_id id)
            {
                if(_done) return;
                emit got_add_list(id);
            }

            void qt_frontend_client::list_add(api::ref_id list_id, api::ref_id widget_id)
            {
                if(_done) return;
                emit got_list_add(list_id, widget_id);
            }

            void qt_frontend_client::list_remove(api::ref_id list_id, api::ref_id widget_id)
            {
                if(_done) return;
                emit got_list_remove(list_id, widget_id);
            }

            size_t qt_frontend_client::list_size(api::ref_id id)
            {
                if(_done) return 0;
                auto p = std::make_shared<std::promise<size_t>>();
                auto f = p->get_future();

                emit got_list_size(id, p);

                return get_until(f, 0);
            }

            void qt_frontend_client::list_clear(api::ref_id id)
            {
                if(_done) return;
                emit got_list_clear(id);
            }

            //pen
            void qt_frontend_client::add_pen(api::ref_id id, const std::string& color, int width)
            {
                if(_done) return;
                emit got_add_pen(id, color, width);
            }

            void qt_frontend_client::pen_set_width(api::ref_id id, int width)
            {
                if(_done) return;
                emit got_pen_set_width(id, width);
            }

            //draw
            void qt_frontend_client::add_draw(api::ref_id id, int width, int height)
            {
                if(_done) return;
                emit got_add_draw(id, width, height);
            }

            void qt_frontend_client::draw_line(api::ref_id id, api::ref_id pen_id, double x1, double y1, double x2, double y2)
            {
                if(_done) return;
                emit got_draw_line(id, pen_id, x1, y1, x2, y2);
            }

            void qt_frontend_client::draw_circle(api::ref_id id, api::ref_id pen_id, double x, double y, double r)
            {
                if(_done) return;
                emit got_draw_circle(id, pen_id, x, y, r);
            }

            void qt_frontend_client::draw_image(api::ref_id id, api::ref_id image_id, double x, double y, double w, double h)
            {
                if(_done) return;
                emit got_draw_image(id, image_id, x, y, w, h);
            }

            void qt_frontend_client::draw_clear(api::ref_id id)
            {
                if(_done) return;
                emit got_draw_clear(id);
            }

            //timer
            void qt_frontend_client::add_timer(api::ref_id id, int msec)
            {
                if(_done) return;
                emit got_add_timer(id, msec);
            }

            bool qt_frontend_client::timer_running(api::ref_id id)
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_timer_running(id, p);

                return get_until(f, false);
            }

            void qt_frontend_client::timer_stop(api::ref_id id)
            {
                if(_done) return;
                emit got_timer_stop(id);
            }

            void qt_frontend_client::timer_start(api::ref_id id)
            {
                if(_done) return;
                emit got_timer_start(id);
            }

            void qt_frontend_client::timer_set_interval(api::ref_id id, int msec)
            {
                if(_done) return;
                emit got_timer_set_interval(id, msec);
            }

            //image
            bool qt_frontend_client::add_image(api::ref_id id, const util::bytes& d)
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_add_image(id, d, p);

                return get_until(f, false);
            }

            int qt_frontend_client::image_width(api::ref_id id)
            {
                if(_done) return 0;
                auto p = std::make_shared<std::promise<int>>();
                auto f = p->get_future();

                emit got_image_width(id, p);

                return get_until(f, 0);
            }

            int qt_frontend_client::image_height(api::ref_id id)
            {
                if(_done) return 0;
                auto p = std::make_shared<std::promise<int>>();
                auto f = p->get_future();

                emit got_image_height(id, p);

                return get_until(f, 0);
            }


            //mic
            void qt_frontend_client::add_mic(api::ref_id id, const std::string& codec)
            {
                if(_done) return;
                emit got_add_mic(id, codec);
            }

            void qt_frontend_client::mic_start(api::ref_id id)
            {
                if(_done) return;
                emit got_mic_start(id);
            }

            void qt_frontend_client::mic_stop(api::ref_id id)
            {
                if(_done) return;
                emit got_mic_stop(id);
            }


            //speaker
            void qt_frontend_client::add_speaker(api::ref_id id, const std::string& codec)
            {
                if(_done) return;
                emit got_add_speaker(id, codec);
            }

            void qt_frontend_client::speaker_mute(api::ref_id id)
            {
                if(_done) return;
                emit got_speaker_mute(id);
            }

            void qt_frontend_client::speaker_unmute(api::ref_id id)
            {
                if(_done) return;
                emit got_speaker_unmute(id);
            }

            void qt_frontend_client::speaker_play(api::ref_id id, const util::bytes& b)
            {
                if(_done) return;
                emit got_speaker_play(id, b);
            }


            //file
            api::file_data qt_frontend_client::open_file()
            {
                if(_done) return api::file_data{};
                auto p = std::make_shared<std::promise<api::file_data>>();
                auto f = p->get_future();

                emit got_open_file(p);

                return f.get();
            }

            api::bin_file_data qt_frontend_client::open_bin_file()
            {
                if(_done) return api::bin_file_data{};
                auto p = std::make_shared<std::promise<api::bin_file_data>>();
                auto f = p->get_future();

                emit got_open_bin_file(p);

                return f.get();
            }

            bool qt_frontend_client::save_file(const std::string& name, const std::string& data)
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_save_file(name, data, p);

                return get_until(f, false);
            }

            bool qt_frontend_client::save_bin_file(const std::string& name, const util::bytes& data)
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_save_bin_file(name, data, p);

                return get_until(f, false);
            }

            //debug
            void qt_frontend_client::print(const std::string& t)
            {
                if(_done) return;
                emit got_print(t);
            }

            //overall gui
            void qt_frontend_client::height(int h)
            {
                if(_done) return;
                emit got_height(h);
            }

            void qt_frontend_client::grow()
            {
                if(_done) return;
                emit got_grow();
            }

            bool qt_frontend_client::visible()
            {
                if(_done) return false;
                auto p = std::make_shared<std::promise<bool>>();
                auto f = p->get_future();

                emit got_visible(p);

                return get_until(f, false);
            }

            void qt_frontend_client::alert()
            {
                if(_done) return;
                emit got_alert();
            }

            //errors
            void qt_frontend_client::report_error(const std::string& e)
            {
                if(_done) return;
                emit got_report_error(e);
            }


            void qt_frontend_client::reset()
            {
                if(_done) return;
                emit got_reset();
            }

            /////////////////////////////////////////////////////////////////
            ////////////////////     SLOTS     //////////////////////////////
            /////////////////////////////////////////////////////////////////
            void qt_frontend_client::do_place(api::ref_id id, int r, int c)
            {
                INVARIANT(_f);
                _f->place(id, r, c);
            }

            void qt_frontend_client::do_place_across(api::ref_id id, int r, int c, int row_span, int col_span)
            {
                INVARIANT(_f);
                _f->place_across(id, r, c, row_span, col_span);
            }

            void qt_frontend_client::do_widget_enable(api::ref_id id, bool b)
            {
                INVARIANT(_f);
                _f->widget_enable(id, b);
            }

            void qt_frontend_client::do_is_widget_enabled(api::ref_id id, bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->is_widget_enabled(id));
            }

            //grid
            void qt_frontend_client::do_add_grid(api::ref_id id)
            {
                INVARIANT(_f);
                _f->add_grid(id);
            }

            void qt_frontend_client::do_grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c)
            {
                INVARIANT(_f);
                _f->grid_place(grid_id, widget_id, r, c);
            }

            void qt_frontend_client::do_grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span)
            {
                INVARIANT(_f);
                _f->grid_place_across(grid_id, widget_id, r, c, row_span, col_span);
            }


            //button
            void qt_frontend_client::do_add_button(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->add_button(id, t);
            }

            void qt_frontend_client::do_button_get_text(api::ref_id id, string_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->button_get_text(id));
            }

            void qt_frontend_client::do_button_set_text(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->button_set_text(id, t);
            }

            void qt_frontend_client::do_button_set_image(api::ref_id id, api::ref_id image_id)
            {
                INVARIANT(_f);
                _f->button_set_image(id, image_id);
            }

            //label
            void qt_frontend_client::do_add_label(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->add_label(id, t);
            }

            void qt_frontend_client::do_label_get_text(api::ref_id id, string_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->label_get_text(id));
            }

            void qt_frontend_client::do_label_set_text(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->label_set_text(id, t);
            }

            //edit
            void qt_frontend_client::do_add_edit(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->add_edit(id, t);
            }

            void qt_frontend_client::do_edit_get_text(api::ref_id id, string_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->edit_get_text(id));
            }

            void qt_frontend_client::do_edit_set_text(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->edit_set_text(id, t);
            }

            //text edit
            void qt_frontend_client::do_add_text_edit(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->add_text_edit(id, t);
            }

            void qt_frontend_client::do_text_edit_get_text(api::ref_id id, string_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->text_edit_get_text(id));
            }

            void qt_frontend_client::do_text_edit_set_text(api::ref_id id, const std::string& t)
            {
                INVARIANT(_f);
                _f->text_edit_set_text(id, t);
            }

            //list
            void qt_frontend_client::do_add_list(api::ref_id id)
            {
                INVARIANT(_f);
                _f->add_list(id);
            }

            void qt_frontend_client::do_list_add(api::ref_id list_id, api::ref_id widget_id)
            {
                INVARIANT(_f);
                _f->list_add(list_id, widget_id);
            }

            void qt_frontend_client::do_list_remove(api::ref_id list_id, api::ref_id widget_id)
            {
                INVARIANT(_f);
                _f->list_remove(list_id, widget_id);
            }

            void qt_frontend_client::do_list_clear(api::ref_id id)
            {
                INVARIANT(_f);
                _f->list_clear(id);
            }

            void qt_frontend_client::do_list_size(api::ref_id id, size_t_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->list_size(id));
            }

            //pen
            void qt_frontend_client::do_add_pen(api::ref_id id, const std::string& color, int width)
            {
                INVARIANT(_f);
                _f->add_pen(id, color, width);
            }

            void qt_frontend_client::do_pen_set_width(api::ref_id id, int width)
            {
                INVARIANT(_f);
                _f->pen_set_width(id, width);
            }

            //draw
            void qt_frontend_client::do_add_draw(api::ref_id id, int width, int height)
            {
                INVARIANT(_f);
                _f->add_draw(id, width, height);
            }

            void qt_frontend_client::do_draw_line(api::ref_id id, api::ref_id pen_id, double x1, double y1, double x2, double y2)
            {
                INVARIANT(_f);
                _f->draw_line(id, pen_id, x1, y1, x2, y2);
            }

            void qt_frontend_client::do_draw_circle(api::ref_id id, api::ref_id pen_id, double x, double y, double r)
            {
                INVARIANT(_f);
                _f->draw_circle(id, pen_id, x, y, r);
            }

            void qt_frontend_client::do_draw_image(api::ref_id id, api::ref_id image_id, double x, double y, double w, double h)
            {
                INVARIANT(_f);
                _f->draw_image(id, image_id, x, y, w, h);
            }

            void qt_frontend_client::do_draw_clear(api::ref_id id)
            {
                INVARIANT(_f);
                _f->draw_clear(id);
            }


            //timer
            void qt_frontend_client::do_add_timer(api::ref_id id, int msec)
            {
                INVARIANT(_f);
                _f->add_timer(id, msec);
            }

            void qt_frontend_client::do_timer_stop(api::ref_id id)
            {
                INVARIANT(_f);
                _f->timer_stop(id);
            }

            void qt_frontend_client::do_timer_start(api::ref_id id)
            {
                INVARIANT(_f);
                _f->timer_start(id);
            }

            void qt_frontend_client::do_timer_set_interval(api::ref_id id, int msec)
            {
                INVARIANT(_f);
                _f->timer_set_interval(id, msec);
            }

            void qt_frontend_client::do_timer_running(api::ref_id id, bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->timer_running(id));
            }

            //image
            void qt_frontend_client::do_add_image(api::ref_id id, const util::bytes& d, bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->add_image(id, d));
            }

            void qt_frontend_client::do_image_width(api::ref_id id, int_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->image_width(id));
            }

            void qt_frontend_client::do_image_height(api::ref_id id, int_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->image_height(id));
            }

            //mic
            void qt_frontend_client::do_add_mic(api::ref_id id, const std::string& codec)
            {
                INVARIANT(_f);
                _f->add_mic(id, codec);
            }

            void qt_frontend_client::do_mic_start(api::ref_id id)
            {
                INVARIANT(_f);
                _f->mic_start(id);
            }

            void qt_frontend_client::do_mic_stop(api::ref_id id)
            {
                INVARIANT(_f);
                _f->mic_stop(id);
            }


            //speaker
            void qt_frontend_client::do_add_speaker(api::ref_id id, const std::string& codec)
            {
                INVARIANT(_f);
                _f->add_speaker(id, codec);
            }

            void qt_frontend_client::do_speaker_mute(api::ref_id id)
            {
                INVARIANT(_f);
                _f->speaker_mute(id);
            }

            void qt_frontend_client::do_speaker_unmute(api::ref_id id)
            {
                INVARIANT(_f);
                _f->speaker_unmute(id);
            }

            void qt_frontend_client::do_speaker_play(api::ref_id id, const util::bytes& b)
            {
                INVARIANT(_f);
                _f->speaker_play(id, b);
            }


            //file
            void qt_frontend_client::do_save_file(const std::string& name, const std::string& data, bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->save_file(name, data));
            }

            void qt_frontend_client::do_save_bin_file(const std::string& name, const util::bytes& data, bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->save_bin_file(name, data));
            }

            void qt_frontend_client::do_open_file(file_data_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->open_file());
            }

            void qt_frontend_client::do_open_bin_file(bin_file_data_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->open_bin_file());
            }

            //debug
            void qt_frontend_client::do_print(const std::string& t)
            {
                INVARIANT(_f);
                _f->print(t);
            }

            //overall gui
            void qt_frontend_client::do_height(int h)
            {
                INVARIANT(_f);
                _f->height(h);
            }

            void qt_frontend_client::do_grow()
            {
                INVARIANT(_f);
                _f->grow();
            }

            void qt_frontend_client::do_visible(bool_promise_ptr p)
            {
                INVARIANT(_f);
                p->set_value(_f->visible());
            }

            void qt_frontend_client::do_alert()
            {
                INVARIANT(_f);
                _f->alert();
            }

            //errors
            void qt_frontend_client::do_report_error(const std::string& e)
            {
                INVARIANT(_f);
                _f->report_error(e);
            }

            void qt_frontend_client::do_reset()
            {
                INVARIANT(_f);
                _f->reset();
            }
        }
    }
}

