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

#include <QtWidgets>

#include "gui/qtw/frontend.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/env.hpp"
#include "util/log.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <QTimer>
#include <QTextBrowser>

#include <functional>
#include <cstdlib>
#include <fstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace a = fire::gui::app;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            namespace
            {
                const std::string SANATIZE_REPLACE = "_";
                const size_t PADDING = 40;
            }

            draw_view::draw_view(
                    qt_frontend* front, 
                    api::backend* back,  
                    api::ref_id id,
                    int width, int height, QWidget* p) : 
                QGraphicsView{p}, 
                _id{id}, 
                _front{front}, 
                _back{back}, 
                _button{0} 
            {
                setScene(new QGraphicsScene{0.0,0.0,static_cast<qreal>(width),static_cast<qreal>(height)});
                setMinimumSize(width,height);
                setMouseTracking(true);
                setRenderHint(QPainter::Antialiasing);
            }

            void draw_view::mousePressEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_back);

                _button = e->button();
                _back->draw_mouse_pressed(_id, _button, e->pos().x(), e->pos().y());
            }

            void draw_view::mouseReleaseEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_back);

                _button = 0;
                _back->draw_mouse_released(_id, _button, e->pos().x(), e->pos().y());
            }

            void draw_view::mouseMoveEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_back);

                if(_button != 0) 
                    _back->draw_mouse_dragged(_id, _button, e->pos().x(), e->pos().y());
                else
                    _back->draw_mouse_moved(_id, e->pos().x(), e->pos().y());
            }

            qt_frontend::qt_frontend(
                    QWidget* c,
                    QGridLayout* cl,
                    list* o ) :
                output{o},
                canvas{c},
                layout{cl}
            {
                REQUIRE(c);
                REQUIRE(cl);

                ENSURE(canvas)
                ENSURE(layout)
            }

            void qt_frontend::set_backend(api::backend* b)
            {
                REQUIRE(b);
                back = b;
            }

            void qt_frontend::place(api::ref_id id, int r, int c)
            {
                INVARIANT(layout);

                auto w = get_widget<QWidget>(id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c);
            }

            void qt_frontend::widget_enable(api::ref_id id, bool enabled)
            {
                auto w = get_widget<QWidget>(id, widgets);
                if(!w) return;

                w->setEnabled(enabled);
            }

            bool qt_frontend::is_widget_enabled(api::ref_id id)
            {
                auto w = get_widget<QWidget>(id, widgets);
                return w ? w->isEnabled() : false;
            }

            void qt_frontend::place_across(api::ref_id id, int r, int c, int row_span, int col_span)
            {
                INVARIANT(layout);

                auto w = get_widget<QWidget>(id, widgets);
                if(!w) return;

                layout->addWidget(w, r, c, row_span, col_span);
            }

            void qt_frontend::add_grid(api::ref_id id)
            {
                INVARIANT(layout);
                INVARIANT(canvas);

                //create widget and new layout
                auto b = new QWidget;
                auto l = new QGridLayout;
                b->setLayout(l);

                //add ref and widget to maps
                widgets[id] = b;
                layouts[id] = l;
            }

            QGridLayout* get_layout(int id, layout_map& map)
            {
                auto lp = map.find(id);
                return lp != map.end() ? lp->second : nullptr;
            }

            void qt_frontend::grid_place(
                    api::ref_id grid_id, api::ref_id widget_id, 
                    int r, int c)
            {
                auto g = get_layout(grid_id, layouts);
                if(!g) return;
                auto w = get_widget<QWidget>(widget_id, widgets);
                if(!w) return;

                g->addWidget(w, r, c);
            }

            void qt_frontend::grid_place_across(
                    api::ref_id grid_id, api::ref_id widget_id, 
                    int r, int c, int row_span, int col_span)
            {
                auto g = get_layout(grid_id, layouts);
                if(!g) return;

                auto w = get_widget<QWidget>(widget_id, widgets);
                if(!w) return;

                g->addWidget(w, r, c, row_span, col_span);
            }

            void qt_frontend::add_button(api::ref_id id, const std::string& text)
            {
                INVARIANT(canvas);

                //create button widget
                auto b = new QPushButton(text.c_str(), canvas);

                //map button to C++ callback
                auto mapper = new QSignalMapper{canvas};
                mapper->setMapping(b, id);
                connect(b, SIGNAL(clicked()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(int)), this, SLOT(button_clicked(int)));

                widgets[id] = b;
            }

            void qt_frontend::button_clicked(int id)
            {
                INVARIANT(back);

                back->button_clicked(id);
            }

            std::string qt_frontend::button_get_text(api::ref_id id)
            {
                auto button = get_widget<QPushButton>(id, widgets);
                CHECK(button);

                return gui::convert(button->text());
            }

            void qt_frontend::button_set_text(api::ref_id id, const std::string& t)
            {
                auto button = get_widget<QPushButton>(id, widgets);
                if(!button) return;

                button->setText(t.c_str());
            }

            void qt_frontend::button_set_image(api::ref_id id, api::ref_id image_id)
            {
                auto button = get_widget<QPushButton>(id, widgets);
                if(!button) return;

                auto image = get_ptr_from_map<QImage_ptr>(image_id, images);
                if(!image) return;

                auto pm = QPixmap::fromImage(*image);
                button->setIcon(pm);
                button->setIconSize(pm.rect().size());
            }

            void qt_frontend::add_label(api::ref_id id, const std::string& text)
            {
                INVARIANT(canvas);

                //create edit widget
                auto w = new QLabel{text.c_str(), canvas};

                //add ref and widget to maps
                widgets[id] = w;
            }

            std::string qt_frontend::label_get_text(api::ref_id id)
            {
                auto l = get_widget<QLabel>(id, widgets);
                return l ? gui::convert(l->text()) : "";
            }

            void qt_frontend::label_set_text(api::ref_id id, const std::string& t)
            {
                auto l = get_widget<QLabel>(id, widgets);
                if(!l) return;

                l->setText(t.c_str());
            }

            void qt_frontend::add_edit(api::ref_id id, const std::string& text)
            {
                INVARIANT(canvas);

                //create edit widget
                auto e = new QLineEdit{text.c_str(), canvas};

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, id);
                connect(e, SIGNAL(textChanged(QString)), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(int)), this, SLOT(edit_edited(int)));

                auto finished_mapper = new QSignalMapper{canvas};
                finished_mapper->setMapping(e, id);
                connect(e, SIGNAL(editingFinished()), finished_mapper, SLOT(map()));
                connect(finished_mapper, SIGNAL(mapped(int)), this, SLOT(edit_finished(int)));

                widgets[id] = e;
            }

            std::string qt_frontend::edit_get_text(api::ref_id id)
            {
                auto l = get_widget<QLineEdit>(id, widgets);
                return l ? gui::convert(l->text()) : "";
            }

            void qt_frontend::edit_set_text(api::ref_id id, const std::string& t)
            {
                auto le = get_widget<QLineEdit>(id, widgets);
                if(!le) return;

                auto pt = gui::convert(le->text());
                if(t != pt) 
                {
                    _can_edit_callback = false;
                    le->setText(t.c_str());
                }
            }

            void qt_frontend::edit_edited(int id)
            {
                INVARIANT(back);
                if(_can_edit_callback) 
                    back->edit_edited(id);

                _can_edit_callback = true;
            }

            void qt_frontend::edit_finished(int id)
            {
                INVARIANT(back);
                back->edit_finished(id);
            }

            void qt_frontend::add_text_edit(api::ref_id id, const std::string& text)
            {
                INVARIANT(canvas);

                //create edit widget
                auto e = new QTextEdit{text.c_str(), canvas};

                //map edit to C++ callback
                auto edit_mapper = new QSignalMapper{canvas};
                edit_mapper->setMapping(e, id);
                connect(e, SIGNAL(textChanged()), edit_mapper, SLOT(map()));
                connect(edit_mapper, SIGNAL(mapped(int)), this, SLOT(text_edit_edited(int)));

                widgets[id] = e;
            }

            std::string qt_frontend::text_edit_get_text(api::ref_id id)
            {
                auto edit = get_widget<QTextEdit>(id, widgets);
                return edit ? gui::convert(edit->toPlainText()) : "";
            }

            void qt_frontend::text_edit_set_text(api::ref_id id, const std::string& t)
            {
                QTextEdit* edit = get_widget<QTextEdit>(id, widgets);
                if(!edit) return;

                auto pt = gui::convert(edit->toPlainText());
                if(t != pt)
                {
                    //save cursor
                    auto pos = edit->textCursor().position();

                    _can_text_callback = false;
                    edit->setText(t.c_str());

                    //put cursor back
                    auto cursor = edit->textCursor();
                    cursor.setPosition(pos);
                    edit->setTextCursor(cursor);
                }
            }

            void qt_frontend::text_edit_edited(int id)
            {
                INVARIANT(back);
                if(_can_text_callback)
                    back->text_edit_edited(id);

                _can_text_callback = true;
            }

            void qt_frontend::add_list(api::ref_id id)
            {
                INVARIANT(canvas);

                //create edit widget
                auto w = new gui::list{canvas};
                w->auto_scroll(true);

                //add ref and widget to maps
                widgets[id] = w;
            }

            void qt_frontend::list_add(api::ref_id list_id, api::ref_id widget_id)
            {
                auto l = get_widget<list>(list_id, widgets);
                if(!l) return;

                auto w = get_widget<QWidget>(widget_id, widgets);
                if(!w) return;

                l->add(w);
            }

            void qt_frontend::list_remove(api::ref_id list_id, api::ref_id widget_id)
            {
                auto l = get_widget<list>(list_id, widgets);
                if(!l) return;

                auto w = get_widget<QWidget>(widget_id, widgets);
                if(!w) return;

                l->remove(w, false);
            }

            size_t qt_frontend::list_size(api::ref_id id)
            {
                auto l = get_widget<list>(id, widgets);
                if(!l) return 0;

                return l->size();
            }

            void qt_frontend::list_clear(api::ref_id id)
            {
                auto l = get_widget<list>(id, widgets);
                if(!l) return;

                l->clear(false); //clear but don't delete widgets
            }

            void qt_frontend::add_pen(api::ref_id id, const std::string& color, int width)
            try
            {
                QPen p{QColor{color.c_str()}};
                p.setWidth(width);
                pens[id] = p;
            }
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in make_pen: " << e.what();
                report_error(s.str());
            }
            catch(...)
            {
                report_error("error in make_pen: unknown");
            }

            void qt_frontend::pen_set_width(api::ref_id id, int width)
            {
                auto p = pens.find(width);
                if(p == pens.end()) return;

                p->second.setWidth(width);
            }

            void qt_frontend::add_draw(api::ref_id id, int width, int height)
            {
                INVARIANT(canvas);
                INVARIANT(back);

                //create edit widget
                auto w = new draw_view{this, back, id, width, height, canvas};

                //add ref and widget to maps
                widgets[id] = w;
            }

            void qt_frontend::draw_line(api::ref_id id, api::ref_id pen_id, double x1, double y1, double x2, double y2)
            {
                auto w = get_widget<draw_view>(id, widgets);
                if(!w) return;
                CHECK(w->scene());

                auto p = pens.find(pen_id);
                if(p == pens.end()) return;

                auto sp1 = w->mapToScene(x1,y1);
                auto sp2 = w->mapToScene(x2,y2);
                w->scene()->addLine(sp1.x(), sp1.y(), sp2.x(), sp2.y(), p->second);
            }

            void qt_frontend::draw_circle(api::ref_id id, api::ref_id pen_id, double x, double y, double r)
            {
                auto w = get_widget<draw_view>(id, widgets);
                if(!w) return; 
                CHECK(w->scene());

                auto p = pens.find(pen_id);
                if(p == pens.end()) return;

                auto sp = w->mapToScene(x,y);
                auto spr = w->mapToScene(x+r,y+r);
                auto rx = std::fabs(spr.x() - sp.x());
                auto ry = std::fabs(spr.y() - sp.y());
                w->scene()->addEllipse(sp.x()-rx, sp.y()-ry, 2*rx, 2*ry, p->second);
            }

            void qt_frontend::draw_image(api::ref_id id, api::ref_id image_id, double x, double y, double w, double h)
            {
                auto v = get_widget<draw_view>(id, widgets);
                if(!v) return; 
                CHECK(v->scene());

                auto image = get_ptr_from_map<QImage_ptr>(image_id, images);
                if(!image) return;

                auto item = v->scene()->addPixmap(QPixmap::fromImage(*image));

                CHECK(item);
                CHECK_GREATER(image->width(), 0);
                CHECK_GREATER(image->height(), 0);

                auto o = v->mapToScene(x,y);
                double sx = w / image->width();
                double sy = h / image->height();

                item->setTransformationMode(Qt::SmoothTransformation);
                QTransform t;
                t.translate(o.x(), o.y());
                t.scale(sx, sy);
                item->setTransform(t);
            }

            void qt_frontend::draw_clear(api::ref_id id)
            {
                auto w = get_widget<draw_view>(id, widgets);
                if(!w) return; 
                CHECK(w->scene());

                w->scene()->clear();
            }

            void qt_frontend::add_timer(api::ref_id id, int msec)
            {
                INVARIANT(canvas);

                //create timer 
                auto t = new QTimer;

                //map timer to C++ callback
                auto timer_mapper = new QSignalMapper{canvas};
                timer_mapper->setMapping(t, id);

                connect(t, SIGNAL(timeout()), timer_mapper, SLOT(map()));
                connect(timer_mapper, SIGNAL(mapped(int)), this, SLOT(timer_triggered(int)));

                //add ref and widget to maps
                timers[id] = t;

                //start timer
                t->start(msec);
            }

            QTimer* get_timer(int id, timer_map& m)
            {
                auto wp = m.find(id);
                if(wp == m.end()) return nullptr;

                CHECK(wp->second);
                return wp->second;
            }

            bool qt_frontend::timer_running(api::ref_id id)
            {
                auto t = get_timer(id, timers);
                return t ? t->isActive() : false;
            }

            void qt_frontend::timer_stop(api::ref_id id)
            {
                auto t = get_timer(id, timers);
                if(!t) return;

                t->stop();
            }

            void qt_frontend::timer_start(api::ref_id id)
            {
                auto t = get_timer(id, timers);
                if(!t) return;

                t->start();
            }

            void qt_frontend::timer_set_interval(api::ref_id id, int msec)
            {
                auto t = get_timer(id, timers);
                if(!t) return;

                t->setInterval(msec);
            }

            void qt_frontend::timer_triggered(int id)
            {
                INVARIANT(back);
                back->timer_triggered(id);
            }

            bool qt_frontend::add_image(api::ref_id id, const util::bytes& d)
            {
                INVARIANT(canvas);
                
                auto i = std::make_shared<QImage>();
                bool loaded = i->loadFromData(reinterpret_cast<const u::ubyte*>(d.data()),d.size());
                if(!loaded) return false;

                auto l = new QLabel;
                l->setPixmap(QPixmap::fromImage(*i));
                l->setMinimumSize(i->width(), i->height());

                widgets[id] = l;
                images[id] = i;
                
                return true;
            }

            int qt_frontend::image_width(api::ref_id id)
            {
                auto image = get_ptr_from_map<QImage_ptr>(id, images);
                return image ? image->width() : 0;
            }

            int qt_frontend::image_height(api::ref_id id)
            {
                auto image = get_ptr_from_map<QImage_ptr>(id, images);
                return image ? image->height() : 0;
            }

            void qt_frontend::add_mic(api::ref_id id, const std::string& codec)
            {
                INVARIANT(back);
                microphone_ptr m{new microphone{back, this, id, codec}};
                mics[id] = m;
            }

            void qt_frontend::mic_start(api::ref_id id)
            {
                auto m = mics.find(id);
                if(m == mics.end()) return;

                m->second->start();
            }

            void qt_frontend::mic_stop(api::ref_id id)
            {
                auto m = mics.find(id);
                if(m == mics.end()) return;

                m->second->stop();
            }

            void qt_frontend::add_speaker(api::ref_id id, const std::string& codec)
            {
                INVARIANT(back);
                speaker_ptr s{new speaker{back, this, codec}};
                spkrs[id] = s;
            }

            void qt_frontend::speaker_mute(api::ref_id id)
            {
                auto s = spkrs.find(id);
                if(s == spkrs.end()) return;

                s->second->mute();
            }

            void qt_frontend::speaker_unmute(api::ref_id id)
            {
                auto s = spkrs.find(id);
                if(s == spkrs.end()) return;

                s->second->unmute();
            }

            void qt_frontend::speaker_play(api::ref_id id, const util::bytes& b)
            {
                auto s = spkrs.find(id);
                if(s == spkrs.end()) return;

                s->second->play(b);
            }

            void qt_frontend::connect_sound(api::ref_id id, QAudioInput* i, QIODevice* d)
            {
                REQUIRE_GREATER_EQUAL(id, 0);
                REQUIRE(i);
                REQUIRE(d);
                INVARIANT(canvas);

                auto mapper = new QSignalMapper{canvas};
                mapper->setMapping(d, id);
                connect(d, SIGNAL(readyRead()), mapper, SLOT(map()));
                connect(mapper, SIGNAL(mapped(int)), this, SLOT(got_sound(int)));
            }

            void qt_frontend::got_sound(int id)
            {
                INVARIANT(back);

                auto mi = mics.find(id);
                if(mi == mics.end()) return;

                auto mic = mi->second;
                CHECK(mic);

                const bool rec = mic->recording();

                u::bytes bd = mic->read_data();
                while(!bd.empty())
                {
                    if(mic->codec() == codec_type::opus) bd = mic->encode(bd);
                    if(rec && !bd.empty()) back->got_sound(id, bd);
                    bd = mic->read_data();
                }
            }
            
            api::file_data qt_frontend::open_file()
            {
                INVARIANT(canvas);

                auto sf = get_file_name(canvas);
                if(sf.empty()) return api::file_data{};
                bf::path p = sf;

                std::ifstream f(sf.c_str());
                if(!f) return api::file_data{};

                std::string data{std::istream_iterator<char>(f), std::istream_iterator<char>()};

                api::file_data fd;
                fd.name = p.filename().string();
                fd.data = std::move(data);
                fd.good = true;
                LOG << "opened file `" << fd.name << " size " << fd.data.size() << std::endl;
                return fd;
            }

            api::bin_file_data qt_frontend::open_bin_file()
            {
                INVARIANT(canvas);
                auto sf = get_file_name(canvas);
                if(sf.empty()) return api::bin_file_data{};
                bf::path p = sf;

                u::bytes bin;
                if(!load_from_file(sf, bin)) return api::bin_file_data{};

                api::bin_file_data fd;
                fd.name = p.filename().string();
                fd.data = std::move(bin);
                fd.good = true;
                LOG << "opened bin file `" << fd.name << " size " << fd.data.size() << std::endl;
                return fd;
            }

            std::string sanatize(const std::string& s)
            {
                const boost::regex SANATIZE_PATH_REGEX("[\\\\\\/\\:]");  //matches \, /, and :
                return boost::regex_replace (s, SANATIZE_PATH_REGEX , SANATIZE_REPLACE);
            }

            bool qt_frontend::save_file(const std::string& suggested_name, const std::string& data)
            {
                if(data.empty()) return false;
                auto home = u::get_home_dir();
                std::string suggested_path = home + "/" + sanatize(suggested_name);
                auto file = QFileDialog::getSaveFileName(canvas, tr("Save File"), suggested_path.c_str());
                if(file.isEmpty()) return false;

                auto fs = convert(file);
                std::ofstream o(fs.c_str());
                if(!o) return false;

                o.write(data.c_str(), data.size());
                LOG << "saved: " << fs << " size " << data.size() <<  std::endl;

                return true;
            }

            bool qt_frontend::save_bin_file(const std::string& suggested_name, const u::bytes& bin)
            {
                if(bin.empty()) return false;
                auto home = u::get_home_dir();
                std::string suggested_path = home + "/" + sanatize(suggested_name);
                auto file = QFileDialog::getSaveFileName(canvas, tr("Save File"), suggested_path.c_str());
                if(file.isEmpty()) return false;

                auto fs = convert(file);
                std::ofstream o(fs.c_str(), std::fstream::out | std::fstream::binary);
                if(!o) return false;

                o.write(bin.data(), bin.size());
                LOG << "saved bin: " << fs << " size " << bin.size() << std::endl;
                return true;
            }

            QWidget* make_error_widget(const std::string& text)
            {
                std::string m = "<b>error:</b> " + text; 
                auto tb = new QTextBrowser;
                tb->setHtml(m.c_str());
                return tb;
            }

            void qt_frontend::report_error(const std::string& e)
            {
                if(!output) return;
                output->add(make_error_widget(e));
            }

            bool qt_frontend::visible() 
            {
                INVARIANT(canvas);
                return !canvas->visibleRegion().isEmpty();
            }

            void qt_frontend::alert() 
            {
                emit alerted();
            }

            void qt_frontend::print(const std::string& m)
            {
                if(!output) return;
                output->add(new QLabel{m.c_str()});
            }

            void delete_timers(timer_map& timers)
            {
                for(auto& t : timers)
                {
                    if(t.second == nullptr) continue;

                    t.second->stop();
                    delete t.second;
                    t.second = nullptr;
                }
                timers.clear();
            }

            void qt_frontend::height(int h)
            {
                INVARIANT(canvas);

                canvas->setMinimumHeight(h);
            }

            void qt_frontend::grow()
            {
            }

            void qt_frontend::reset()
            {
                INVARIANT(layout);

                //delete widgets without parent
                delete_timers(timers);

                //clear widgets
                QLayoutItem *c = nullptr;

                while((c = layout->takeAt(0)) != nullptr)
                {
                    if(c->widget()) delete c->widget();
                    delete c;
                }

                if(output) output->clear();
                images.clear();
                widgets.clear();

                ENSURE(widgets.empty());
                ENSURE(timers.empty());
                ENSURE(images.empty());
                ENSURE_EQUAL(layout->count(), 0);
            }
        }
    }
}

