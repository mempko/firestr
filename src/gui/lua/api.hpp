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

#ifndef FIRESTR_APP_LUA_SCRIPT_API_H
#define FIRESTR_APP_LUA_SCRIPT_API_H

#include "gui/lua/base.hpp"
#include "gui/lua/widgets.hpp"
#include "gui/lua/audio.hpp"
#include "gui/app/app.hpp"
#include "gui/api/service.hpp"
#include "conversation/conversation_service.hpp"

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            using script_ptr = std::shared_ptr<SLB::Script>;
            using callback_map = std::unordered_map<std::string, std::string>;

            struct error_info
            {
                int line;
                std::string message;
            };
            using error_infos = std::vector<error_info>;

            class lua_api : public api::backend
            {
                public:
                    lua_api(
                            app::app_ptr a,
                            messages::sender_ptr sender,
                            conversation::conversation_ptr conversation,
                            conversation::conversation_service_ptr conversation_service,
                            api::frontend*);

                    ~lua_api();

                public:
                    virtual void button_clicked(api::ref_id);
                    virtual void edit_edited(api::ref_id id);
                    virtual void edit_finished(api::ref_id id);
                    virtual void text_edit_edited(api::ref_id id);
                    virtual void timer_triggered(api::ref_id id);
                    virtual void got_sound(api::ref_id id, const util::bytes&);
                    virtual void draw_mouse_pressed(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_released(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_dragged(api::ref_id, int button, int x, int y);
                    virtual void draw_mouse_moved(api::ref_id, int x, int y);

                public:
                    //misc
                    app::app_ptr app;
                    store_ref_ptr local_data;
                    store_ref_ptr data;
                    error_infos errors; 
                    SLB::Manager manager;
                    script_ptr state;
                    std::string who_started_id;

                    api::frontend* front;

                    //message
                    conversation::conversation_ptr conversation;
                    conversation::conversation_service_ptr conversation_service;
                    messages::sender_ptr sender;

                    std::string message_callback;
                    std::string local_message_callback;
                    callback_map message_callbacks;
                    callback_map local_message_callbacks;

                    //util
                    void bind();
                    error_info execute(const std::string&);
                    void run(const std::string&);
                    void reset_widgets();
                    void message_received(const script_message&);
                    void event_received(const event_message&);
                    void send_to_helper(user::user_info_ptr, const script_message&); 

                    button_ref_map button_refs;
                    label_ref_map label_refs;
                    edit_ref_map edit_refs;
                    text_edit_ref_map text_edit_refs;
                    list_ref_map list_refs;
                    grid_ref_map grid_refs;
                    draw_ref_map draw_refs;
                    timer_ref_map timer_refs;
                    image_ref_map image_refs;
                    microphone_ref_map mic_refs;
                    speaker_ref_map speaker_refs;

                    //id functions
                    api::ref_id ids = 0;
                    api::ref_id new_id();

                    //observable functions
                    observable_ref_name_map observable_names;
                    observable_ref* get_observable(int id);

                    //error func
                    void report_error(const std::string& e, int line = -1);
                    error_info get_error() const;

                    //================================
                    //API
                    //================================

                    //gui
                    void print(const std::string& a);
                    void alert();
                    button_ref make_button(const std::string& title);
                    label_ref make_label(const std::string& text);
                    edit_ref make_edit(const std::string& text);
                    text_edit_ref make_text_edit(const std::string& text);
                    list_ref make_list();
                    draw_ref make_draw(int width, int height);
                    pen_ref make_pen(const std::string& color, int width);
                    timer_ref make_timer(int msec, const std::string& callback);
                    image_ref make_image(const bin_data& data);

                    //multimedia
                    microphone_ref make_mic(const std::string& callback, const std::string& codec);
                    speaker_ref make_speaker(const std::string& codec);
                    opus_encoder_wrapper make_audio_encoder();
                    opus_decoder_wrapper make_audio_decoder();
                    vclock_wrapper make_vclock();

                    //grid
                    grid_ref make_grid();
                    void place(const widget_ref& w, int r, int c);
                    void place_across(const widget_ref& w, int r, int c, int row_span, int col_span);

                    //size
                    void height(int h);
                    void grow();

                    //messages
                    void set_message_callback(const std::string& a);
                    void set_local_message_callback(const std::string& a);
                    void set_message_callback_by_type(
                            const std::string& t, 
                            const std::string& a);
                    void set_local_message_callback_by_type(
                            const std::string& t, 
                            const std::string& a);

                    script_message make_message();
                    void send(const event_message&);
                    void send_simple_event(const std::string& name, const std::string& type);
                    void send_all(const script_message&);
                    void send_to(const contact_ref&, const script_message&); 
                    void send_local(const script_message& m);

                    //contacts
                    size_t total_contacts() const;
                    int last_contact() const;
                    contact_ref get_contact(size_t);
                    contact_ref who_started();
                    contact_ref self();

                    //apps
                    size_t total_apps() const;
                    app_ref get_app(size_t);
                    bool launched_local() const;

                    //file
                    file_data_wrapper open_file();
                    bin_file_data_wrapper open_bin_file();
                    bool save_file(const std::string& suggested_name, const std::string& data);
                    bool save_bin_file(const std::string& suggested_name, const bin_data& data);
                    void connect_sound(int id, QAudioInput*, QIODevice*); 

                private:
                    bool visible() const;

                private:
                        error_info _error;
            };

            using lua_api_ptr = std::shared_ptr<lua_api>;

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


