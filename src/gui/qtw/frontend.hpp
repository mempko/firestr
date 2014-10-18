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

#ifndef FIRESTR_GUI_QTW_H
#define FIRESTR_GUI_QTW_H

#include "gui/app/app.hpp"
#include "gui/api/service.hpp"
#include "gui/qtw/audio.hpp"
#include "gui/list.hpp"
#include "conversation/conversation_service.hpp"

#include <QImage>
#include <QGraphicsView>

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            using QImage_ptr = std::shared_ptr<QImage>;
            using widget_map = std::unordered_map<api::ref_id, QWidget*>;
            using image_map = std::unordered_map<api::ref_id, QImage_ptr>;
            using layout_map = std::unordered_map<api::ref_id, QGridLayout*>;
            using timer_map = std::unordered_map<api::ref_id, QTimer*>;
            using callback_map = std::unordered_map<std::string, std::string>;
            using pen_map = std::unordered_map<api::ref_id, QPen>;
            using mic_map = std::unordered_map<api::ref_id, microphone_ptr>;
            using spk_map = std::unordered_map<api::ref_id, speaker_ptr>;

            class qt_frontend;

            class draw_view : public QGraphicsView
            {
                Q_OBJECT
                public:
                    draw_view(qt_frontend*,
                            api::backend*, 
                            api::ref_id,
                            int width, 
                            int height, 
                            QWidget* parent = nullptr);

                protected:
                    void mousePressEvent(QMouseEvent*);
                    void mouseReleaseEvent(QMouseEvent*);
                    void mouseMoveEvent(QMouseEvent*);

                private:
                    api::ref_id _id;
                    qt_frontend* _front;
                    api::backend* _back;
                    int _button;
            };

            class qt_frontend : public QObject, public api::frontend
            {
                Q_OBJECT

                public:
                    qt_frontend(QWidget* c, QGridLayout* cl, list* output);

                public:
                    void set_backend(api::backend*);

                public:
                    //all widgets
                    virtual void place(api::ref_id, int r, int c);
                    virtual void place_across(api::ref_id id, int r, int c, int row_span, int col_span);
                    virtual void widget_enable(api::ref_id, bool);
                    virtual bool is_widget_enabled(api::ref_id);

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
                    virtual void draw_line(api::ref_id id, api::ref_id pen_id, double x1, double y1, double x2, double y2);
                    virtual void draw_circle(api::ref_id id, api::ref_id pen_id, double x, double y, double r);
                    virtual void draw_image(api::ref_id id, api::ref_id image_id, double x, double y, double w, double h);
                    virtual void draw_clear(api::ref_id id);

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

                    //errors
                    virtual void report_error(const std::string& e);

                    virtual void reset();

                public slots:
                    void button_clicked(int id);
                    void edit_edited(int id);
                    void edit_finished(int id);
                    void text_edit_edited(int id);
                    void timer_triggered(int id);
                    void got_sound(int id);

                public:
                    void connect_sound(api::ref_id id, QAudioInput* i, QIODevice* d);

                private:
                    friend class microphone;
                    friend class speaker;

                private:
                    //all widgets referenced are stored here
                    layout_map layouts;
                    widget_map widgets;
                    image_map images;
                    timer_map timers;
                    pen_map pens;
                    mic_map mics;
                    spk_map spkrs;

                    list* output = nullptr;
                    QWidget* canvas = nullptr;
                    QGridLayout* layout = nullptr;
                    api::backend* back = nullptr;
            };

            using qt_frontend_ptr = std::shared_ptr<qt_frontend>;

            template<class W, class M>
                W* get_widget(int id, M& map)
                {
                    auto wp = map.find(id);
                    return wp != map.end() ? dynamic_cast<W*>(wp->second) : nullptr;
                }

            template<class W, class M>
                W get_ptr_from_map(int id, M& map)
                {
                    auto wp = map.find(id);
                    return wp != map.end() ? wp->second : nullptr;
                }
        }
    }
}

#endif


