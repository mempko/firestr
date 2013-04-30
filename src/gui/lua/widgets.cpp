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

#include <QtGui>

#include "gui/lua/widgets.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QTimer>

#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::session;
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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->button_refs.find(id);
                if(rp == api->button_refs.end()) return;

                auto button = get_widget<QPushButton>(id, api->widgets);
                CHECK(button);

                button->setText(t.c_str());
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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->label_refs.find(id);
                if(rp == api->label_refs.end()) return;

                auto l = get_widget<QLabel>(id, api->widgets);
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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->edit_refs.find(id);
                if(rp == api->edit_refs.end()) return;

                auto edit = get_widget<QLineEdit>(id, api->widgets);
                CHECK(edit);

                edit->setText(t.c_str());
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
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                auto edit = get_widget<QTextEdit>(id, api->widgets);
                CHECK(edit);

                edit->setText(t.c_str());
            }

            void text_edit_ref::set_edited_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto rp = api->text_edit_refs.find(id);
                if(rp == api->text_edit_refs.end()) return;

                rp->second.edited_callback = c;
                edited_callback = c;
            }

            void list_ref::add(const widget_ref& wr)
            {
                REQUIRE_FALSE(wr.id == 0);
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto l = get_widget<gui::list>(id, api->widgets);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

                l->add(w);
            }

            void list_ref::clear()
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto l = get_widget<gui::list>(id, api->widgets);
                if(!l) return;

                l->clear();
            }

            QGridLayout* get_layout(int id, layout_map& map)
            {
                auto lp = map.find(id);
                return lp != map.end() ? lp->second : nullptr;
            }

            void canvas_ref::place(const widget_ref& wr, int r, int c)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto l = get_layout(id, api->layouts);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

                l->addWidget(w, r, c);
            }

            void canvas_ref::place_across(const widget_ref& wr, int r, int c, int row_span, int col_span)
            {
                INVARIANT(api);
                INVARIANT_FALSE(id == 0);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto l = get_layout(id, api->layouts);
                if(!l) return;

                auto w = get_widget<QWidget>(wr.id, api->widgets);
                if(!w) return;

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
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in mouse_pressed: " << e.what();
                api->report_error(s.str());
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
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in mouse_released: " << e.what();
                api->report_error(s.str());
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
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in mouse_moved: " << e.what();
                api->report_error(s.str());
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
            catch(std::exception& e)
            {
                std::stringstream s;
                s << "error in mouse_dragged: " << e.what();
                api->report_error(s.str());
            }
            catch(...)
            {
                api->report_error("error in mouse_dragged: unknown");
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

            void draw_ref::clear()
            {
                INVARIANT(api);

                auto w = get_view();
                if(!w) return;

                w->scene()->clear();
            }

            draw_view::draw_view(draw_ref ref, int width, int height) : 
                QGraphicsView(), _ref{ref}, _button{0}
            {
                setScene(new QGraphicsScene{0.0,0.0,width,height});
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
                ref->second.mouse_pressed(e->button(), e->pos().x(), e->pos().y());

            }
            void draw_view::mouseReleaseEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_ref.api);

                auto ref = _ref.api->draw_refs.find(_ref.id);
                if(ref == _ref.api->draw_refs.end()) return;

                _button = 0;
                ref->second.mouse_released(e->button(), e->pos().x(), e->pos().y());
            }

            void draw_view::mouseMoveEvent(QMouseEvent* e)
            {
                if(!e) return;

                INVARIANT(_ref.api);

                auto ref = _ref.api->draw_refs.find(_ref.id);
                if(ref == _ref.api->draw_refs.end()) return;

                ref->second.mouse_moved(e->pos().x(), e->pos().y());
                if(_button != 0) ref->second.mouse_dragged(_button, e->pos().x(), e->pos().y());
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
        }
    }
}
