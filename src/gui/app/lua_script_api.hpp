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

#ifndef FIRESTR_APP_LUA_SCRIPT_API_H
#define FIRESTR_APP_LUA_SCRIPT_API_H

#ifndef Q_MOC_RUN
#include "gui/list.hpp"
#include "gui/message.hpp"
#include "session/session.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"
#endif

#include <QObject>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSignalMapper>
#include <QGraphicsView>

#include "slb/SLB.hpp"

#include <string>
#include <unordered_map>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class lua_script_api;

            struct basic_ref
            {
                int id;
                lua_script_api* api;
            };

            struct widget_ref : public basic_ref
            {
                bool enabled(); 
                void enable();
                void disable();
            };

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

            struct contact_ref : public basic_ref
            {
                std::string user_id;
                std::string get_name() const;
                bool is_online() const;
            };

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

            class script_message;
            struct app_ref : public basic_ref
            {
                std::string app_id;
                std::string get_id() const;
                void send(const script_message&); 
            };

            extern const std::string SCRIPT_MESSAGE;
            class script_message
            {
                public:
                    script_message(lua_script_api*);
                    script_message(const fire::message::message&, lua_script_api*);
                    operator fire::message::message() const;

                public:
                    std::string get(const std::string&) const;
                    void set(const std::string&, const std::string&);
                    contact_ref from() const;
                    bool is_local() const;
                    app_ref app() const;

                private:
                    std::string _from_id;
                    std::string _local_app_id;
                    util::dict _v;
                    lua_script_api* _api;
            };

            using script_ptr = std::shared_ptr<SLB::Script>;
            using widget_map = std::unordered_map<int, QWidget*>;
            using layout_map = std::unordered_map<int, QGridLayout*>;

            class lua_script_api : public QObject
            {
                Q_OBJECT
                public:
                    lua_script_api(
                            const user::contact_list& con,
                            messages::sender_ptr sender,
                            session::session_ptr session,
                            QWidget* can,
                            QGridLayout* lay,
                            list* out = nullptr);

                public:
                    list* output;
                    QWidget* canvas;
                    QGridLayout* layout;
                    SLB::Manager manager;
                    script_ptr state;

                    //message
                    user::contact_list contacts;
                    session::session_ptr session;
                    messages::sender_ptr sender;
                    std::string message_callback;
                    std::string local_message_callback;

                    //util
                    void bind();
                    std::string execute(const std::string&);
                    void run(const std::string&);
                    void reset_widgets();
                    void message_recieved(const script_message&);

                    button_ref_map button_refs;
                    label_ref_map label_refs;
                    edit_ref_map edit_refs;
                    text_edit_ref_map text_edit_refs;
                    list_ref_map list_refs;
                    canvas_ref_map canvas_refs;
                    draw_ref_map draw_refs;

                    //all widgets referenced are stored here
                    layout_map layouts;
                    widget_map widgets;

                    //id functions
                    int ids;
                    int new_id();

                    //error func
                    void report_error(const std::string& e);

                    //================================
                    //API
                    //================================
                    void print(const std::string& a);
                    button_ref make_button(const std::string& title);
                    label_ref make_label(const std::string& text);
                    edit_ref make_edit(const std::string& text);
                    text_edit_ref make_text_edit(const std::string& text);
                    list_ref make_list();
                    draw_ref make_draw(int width, int height);
                    QPen make_pen(const std::string& color, int width);

                    canvas_ref make_canvas(int r, int c);
                    void place(const widget_ref& w, int r, int c);
                    void place_across(const widget_ref& w, int r, int c, int row_span, int col_span);

                    void set_message_callback(const std::string& a);
                    void set_local_message_callback(const std::string& a);

                    script_message make_message();
                    void send_all(const script_message&);
                    void send_to(const contact_ref&, const script_message&); 
                    void send_local(const script_message& m);

                    size_t total_contacts() const;
                    int last_contact() const;
                    contact_ref get_contact(size_t);

                    size_t total_apps() const;
                    app_ref get_app(size_t);

                    public slots:
                        void button_clicked(int id);
                        void edit_edited(int id);
                        void edit_finished(int id);
                        void text_edit_edited(int id);
            };

            using lua_script_api_ptr = std::shared_ptr<lua_script_api>;

        }
    }
}

#endif

