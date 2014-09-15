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
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#include <QtWidgets>

#include "gui/lua/widgets.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>
#include <QIcon>
#include <QTransform>

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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return "";

                auto button = get_widget<QPushButton>(id, api->widgets);
                CHECK(button);

                return gui::convert(button->text());
            }

            void button_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                QPushButton* button = nullptr;
                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    auto rp = api->button_refs.find(id);
                    if(rp == api->button_refs.end()) return;

                    button = get_widget<QPushButton>(id, api->widgets);
                }

                CHECK(button);
                button->setText(t.c_str());
            }

            void button_ref::set_image(const image_ref& i)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget<QPushButton>(id, api->widgets);
                CHECK(button);

                auto image = get_ptr_from_map<QImage_ptr>(i.id, api->images);
                if(!image) return;

                auto pm = QPixmap::fromImage(*image);
                button->setIcon(pm);
                button->setIconSize(pm.rect().size());
            }

            void button_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->label_refs.find(id);
                if(rp == api->label_refs.end()) return "";

                auto l = get_widget<QLabel>(id, api->widgets);
                CHECK(l);

                return gui::convert(l->text());
            }

            void label_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                QLabel* l = nullptr;
                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    auto rp = api->label_refs.find(id);
                    if(rp == api->label_refs.end()) return;

                    l = get_widget<QLabel>(id, api->widgets);
                    if(!l) return;
                }

                CHECK(l);
                l->setText(t.c_str());
            }

            std::string edit_ref::get_text() const
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return "";

                auto edit = get_widget<QLineEdit>(id, api->widgets);
                CHECK(edit);

                return gui::convert(edit->text());
            }

            void edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);

                QLineEdit* edit = nullptr;
                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    auto rp = api->edit_refs.find(id);
                    if(rp == api->edit_refs.end()) return;

                    edit = get_widget<QLineEdit>(id, api->widgets);
                }

                CHECK(edit);
                auto pt = gui::convert(edit->text());
                if(t != pt) edit->setText(t.c_str());
            }

            void edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            void edit_ref::set_finished_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return "";

                auto edit = get_widget<QTextEdit>(id, api->widgets);
                CHECK(edit);

                return gui::convert(edit->toPlainText());
            }

            void text_edit_ref::set_text(const std::string& t)
            {
                INVARIANT(api);
                QTextEdit* edit = nullptr;
                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    auto rp = api->text_edit_refs.find(id);
                    if(rp == api->text_edit_refs.end()) return;

                    edit = get_widget<QTextEdit>(id, api->widgets);
                }

                CHECK(edit);
                auto pt = gui::convert(edit->toPlainText());
                if(t != pt)
                {
                    //save cursor
                    auto pos = edit->textCursor().position();

                    edit->setText(t.c_str());

                    //put cursor back
                    auto cursor = edit->textCursor();
                    cursor.setPosition(pos);
                    edit->setTextCursor(cursor);
                }
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

            list* list_ref::get_list() const
            {
                std::lock_guard<std::mutex> lock(api->mutex);
                return get_widget<gui::list>(id, api->widgets);
            }


            list_ref::list_widget list_ref::get_both(const widget_ref& r)
            {
                std::lock_guard<std::mutex> lock(api->mutex);

                auto l = get_widget<gui::list>(id, api->widgets);
                auto w = get_widget<QWidget>(r.id, api->widgets);

                return std::make_pair(l, w);
            }

            void list_ref::add(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id == 0);
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                auto f = get_both(wr);

                gui::list* l = f.first;
                if(!l) return;

                QWidget* w = f.second;
                if(!w) return;

                CHECK(l);
                CHECK(w);
                l->add(w);
            }

            void list_ref::remove(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id == 0);
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                auto f = get_both(wr);

                auto l = f.first;
                if(!l) return;

                auto w = f.second;
                if(!w) return;

                CHECK(l);
                CHECK(w);
                l->remove(w, false);
            }

            size_t list_ref::size() const
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                auto l = get_list();
                if(!l) return 0;

                CHECK(l);
                return l->size();
            }

            void list_ref::clear()
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                gui::list* l = get_list();
                if(!l) return;

                CHECK(l);
                l->clear(false); //clear but don't delete widgets
            }

            QGridLayout* get_layout(int id, layout_map& map)
            {
                auto lp = map.find(id);
                return lp != map.end() ? lp->second : nullptr;
            }

            void grid_ref::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                QGridLayout* l = nullptr;
                QWidget* w = nullptr;

                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    l = get_layout(id, api->layouts);
                    if(!l) return;

                    w = get_widget<QWidget>(wr.id, api->widgets);
                    if(!w) return;
                }

                CHECK(l);
                CHECK(w);

                l->addWidget(w, r, c);
            }

            void grid_ref::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);

                QGridLayout* l = nullptr;
                QWidget* w = nullptr;

                {
                    std::lock_guard<std::mutex> lock(api->mutex);

                    l = get_layout(id, api->layouts);
                    if(!l) return;

                    w = get_widget<QWidget>(wr.id, api->widgets);
                    if(!w) return;
                }

                CHECK(l);
                CHECK(w);

                l->addWidget(w, r, c, row_span, col_span);
            }

            void draw_ref::set_mouse_released_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_released_callback = c;
                mouse_released_callback = c;
            }  

            void draw_ref::set_mouse_pressed_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_pressed_callback = c;
                mouse_pressed_callback = c;
            }  

            void draw_ref::set_mouse_moved_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_moved_callback = c;
                mouse_moved_callback = c;
            }  

            void draw_ref::set_mouse_dragged_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->draw_refs.find(id);
                if(rp == api->draw_refs.end()) return;

                rp->second.mouse_dragged_callback = c;
                mouse_dragged_callback = c;
            }  

            void draw_ref::set_pen(QPen p)
            {
                INVARIANT(api);

                std::lock_guard<std::mutex> lock(api->mutex);

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

            draw_view* draw_ref::get_view()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto dp = api->draw_refs.find(id);
                if(dp == api->draw_refs.end()) return nullptr;

                auto w = get_widget<draw_view>(id, api->widgets);
                if(w) CHECK(w->scene());
                return w;
            }

            void draw_ref::line(double x1, double y1, double x2, double y2)
            {
                INVARIANT(api);

                auto w = get_view();
                if(!w) return;

                auto sp1 = w->mapToScene(x1,y1);
                auto sp2 = w->mapToScene(x2,y2);
                w->scene()->addLine(sp1.x(), sp1.y(), sp2.x(), sp2.y(), pen);
            }

            void draw_ref::circle(double x, double y, double r)
            {
                INVARIANT(api);

                auto w = get_view();
                if(!w) return;

                auto sp = w->mapToScene(x,y);
                auto spr = w->mapToScene(x+r,y+r);
                auto rx = std::fabs(spr.x() - sp.x());
                auto ry = std::fabs(spr.y() - sp.y());
                w->scene()->addEllipse(sp.x()-rx, sp.y()-ry, 2*rx, 2*ry, pen);
            }

            void draw_ref::image(const image_ref& i, double x, double y, double w, double h)
            {
                INVARIANT(api);

                auto v = get_view();
                if(!v) return;
                if(!i.good()) return;

                auto image = get_ptr_from_map<QImage_ptr>(i.id, api->images);
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

            void draw_ref::clear()
            {
                INVARIANT(api);

                auto w = get_view();
                if(!w) return;

                w->scene()->clear();
            }

            draw_view::draw_view(draw_ref ref, int width, int height, QWidget* p) : 
                QGraphicsView{p}, _ref{ref}, _button{0}
            {
                setScene(new QGraphicsScene{0.0,0.0,static_cast<qreal>(width),static_cast<qreal>(height)});
                setMinimumSize(width,height);
                setMouseTracking(true);
                setRenderHint(QPainter::Antialiasing);
            }

            void draw_view::mousePressEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_ref.api);

                auto ref = _ref.api->draw_refs.find(_ref.id);
                if(ref == _ref.api->draw_refs.end()) return;

                _button = e->button();
                auto name = ref->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["b"] = _button;
                    d["x"] = e->pos().x();
                    d["y"] = e->pos().y();
                    event_message em{name, "p", d, _ref.api};
                    _ref.api->send(em);
                }
                ref->second.mouse_pressed(e->button(), e->pos().x(), e->pos().y());
            }

            void draw_view::mouseReleaseEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_ref.api);

                auto ref = _ref.api->draw_refs.find(_ref.id);
                if(ref == _ref.api->draw_refs.end()) return;

                _button = 0;
                auto name = ref->second.get_name();
                if(!name.empty())
                {
                    u::dict d;
                    d["b"] = _button;
                    d["x"] = e->pos().x();
                    d["y"] = e->pos().y();
                    event_message em{name, "r", d, _ref.api};
                    _ref.api->send(em);
                }
                ref->second.mouse_released(e->button(), e->pos().x(), e->pos().y());
            }

            void draw_view::mouseMoveEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_ref.api);

                auto ref = _ref.api->draw_refs.find(_ref.id);
                if(ref == _ref.api->draw_refs.end()) return;

                auto name = ref->second.get_name();
                if(_button != 0) 
                {
                    if(!name.empty())
                    {
                        u::dict d;
                        d["b"] = _button;
                        d["x"] = e->pos().x();
                        d["y"] = e->pos().y();
                        event_message em{name, "d", d, _ref.api};
                        _ref.api->send(em);
                    }
                    ref->second.mouse_dragged(_button, e->pos().x(), e->pos().y());
                }
                else
                {
                    if(!name.empty())
                    {
                        u::dict d;
                        d["x"] = e->pos().x();
                        d["y"] = e->pos().y();
                        event_message em{name, "m", d, _ref.api};
                        _ref.api->send(em);
                    }

                    ref->second.mouse_moved(e->pos().x(), e->pos().y());
                }
            }

            QTimer* get_timer(int id, timer_map& m)
            {
                auto wp = m.find(id);
                if(wp == m.end()) return nullptr;

                CHECK(wp->second);
                return wp->second;
            }

            bool timer_ref::running()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto t = get_timer(id, api->timers);
                return t ? t->isActive() : false;
            }

            void timer_ref::stop()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto t = get_timer(id, api->timers);
                if(!t) return;

                t->stop();
            }

            void timer_ref::start()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto t = get_timer(id, api->timers);
                if(!t) return;

                t->start();
            }

            void timer_ref::set_interval(int msec)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto t = get_timer(id, api->timers);
                if(!t) return;

                t->setInterval(msec);
            }

            void timer_ref::set_callback(const std::string& c)  
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->timer_refs.find(id);
                if(rp == api->timer_refs.end()) return;

                rp->second.callback = c;
                callback = c;
            }

            int image_ref::width() const
            {
                return w;
            }

            int image_ref::height() const
            {
                return h;
            }

            bool image_ref::good() const
            {
                return g;
            }
        }
    }
}
