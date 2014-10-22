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

#include "gui/lua/backend_client.hpp"
#include "message/message.hpp"
#include "util/serialize.hpp"

namespace u = fire::util;
namespace m = fire::message;
namespace s = fire::service;

using ref_id = fire::gui::api::ref_id;

namespace fire 
{
    namespace gui 
    {
        namespace lua 
        {
            namespace
            {
                const std::string RUN_CODE = "r_c";
                const std::string BUTTON_CLICKED = "b_c";
                const std::string EDIT_EDITED = "e_e";
                const std::string EDIT_FINISHED = "e_f";
                const std::string TEXT_EDIT_EDITED = "t_e";
                const std::string TIMER_TRIGGERED = "t_t";
                const std::string GOT_SOUND = "g_s";
                const std::string DRAW_MOUSE_PRESSED = "d_p";
                const std::string DRAW_MOUSE_RELEASED = "d_r";
                const std::string DRAW_MOUSE_DRAGGED = "d_d";
                const std::string DRAW_MOUSE_MOVED = "d_m";
            }

            f_message(run_code_msg)
            {
                std::string code;
                f_message_init(run_code_msg, RUN_CODE);
                f_serialize
                {
                    f_s(code);
                }
            };

            f_message(button_clicked_msg)
            {
                ref_id id;
                f_message_init(button_clicked_msg, BUTTON_CLICKED);
                f_serialize
                {
                    f_s(id);
                }
            };

            f_message(edit_edited_msg)
            {
                ref_id id;
                f_message_init(edit_edited_msg, EDIT_EDITED);
                f_serialize
                {
                    f_s(id);
                }
            };

            f_message(edit_finished_msg)
            {
                ref_id id;
                f_message_init(edit_finished_msg, EDIT_FINISHED);
                f_serialize
                {
                    f_s(id);
                }
            };

            f_message(text_edit_edited_msg)
            {
                ref_id id;
                f_message_init(text_edit_edited_msg, TEXT_EDIT_EDITED);
                f_serialize
                {
                    f_s(id);
                }
            };

            f_message(timer_triggered_msg)
            {
                ref_id id;
                f_message_init(timer_triggered_msg, TIMER_TRIGGERED);
                f_serialize
                {
                    f_s(id);
                }
            };

            f_message(got_sound_msg)
            {
                ref_id id;
                u::bytes data;
                f_message_init(got_sound_msg, GOT_SOUND);
                f_serialize
                {
                    f_s(id);
                    f_sk("d", data);
                }
            };

            f_message(draw_mouse_pressed_msg)
            {
                ref_id id;
                int button;
                int x;
                int y;

                f_message_init(draw_mouse_pressed_msg, DRAW_MOUSE_PRESSED);
                f_serialize
                {
                    f_s(id);
                    f_sk("b", button);
                    f_s(x);
                    f_s(y);
                }
            };

            f_message(draw_mouse_released_msg)
            {
                ref_id id;
                int button;
                int x;
                int y;

                f_message_init(draw_mouse_released_msg, DRAW_MOUSE_RELEASED);
                f_serialize
                {
                    f_s(id);
                    f_sk("b", button);
                    f_s(x);
                    f_s(y);
                }
            };

            f_message(draw_mouse_dragged_msg)
            {
                ref_id id;
                int button;
                int x;
                int y;

                f_message_init(draw_mouse_dragged_msg, DRAW_MOUSE_DRAGGED);
                f_serialize
                {
                    f_s(id);
                    f_sk("b", button);
                    f_s(x);
                    f_s(y);
                }
            };

            f_message(draw_mouse_moved_msg)
            {
                ref_id id;
                int x;
                int y;

                f_message_init(draw_mouse_moved_msg, DRAW_MOUSE_MOVED);
                f_serialize
                {
                    f_s(id);
                    f_s(x);
                    f_s(y);
                }
            };

            backend_client::backend_client(lua_api* api, m::mailbox_ptr m) : 
                s::service{m},
                _api{api} 
            {
                REQUIRE(api);
                REQUIRE(m);

                init_handlers();

                ENSURE(_api);
            }

            void backend_client::init_handlers()
            {
                INVARIANT(_api);

                using std::bind;
                using namespace std::placeholders;

                handle(SCRIPT_MESSAGE, 
                        bind(&backend_client::received_script_message, this, _1));
                handle(EVENT_MESSAGE,
                        bind(&backend_client::received_event_message, this, _1));

                handle(RUN_CODE, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            run_code_msg b;
                            b.from_message(m);
                            _api->run(b.code);
                        });

                handle(BUTTON_CLICKED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            button_clicked_msg b;
                            b.from_message(m);
                            _api->button_clicked(b.id);
                        });

                handle(EDIT_EDITED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            edit_edited_msg e;
                            e.from_message(m);
                            _api->edit_edited(e.id); 
                        });

                handle(EDIT_FINISHED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            edit_finished_msg e;
                            e.from_message(m);
                            _api->edit_finished(e.id);
                        });

                handle(TEXT_EDIT_EDITED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            text_edit_edited_msg e;
                            e.from_message(m);
                            _api->text_edit_edited(e.id);
                        });

                handle(TIMER_TRIGGERED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            timer_triggered_msg e;
                            e.from_message(m);
                            _api->timer_triggered(e.id);
                        });

                handle(GOT_SOUND, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            got_sound_msg e;
                            e.from_message(m);
                            _api->got_sound(e.id, e.data);
                        });

                handle(DRAW_MOUSE_PRESSED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            draw_mouse_pressed_msg e;
                            e.from_message(m);
                            _api->draw_mouse_pressed(e.id, e.button, e.x, e.y);
                        });

                handle(DRAW_MOUSE_RELEASED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            draw_mouse_released_msg e;
                            e.from_message(m);
                            _api->draw_mouse_released(e.id, e.button, e.x, e.y);
                        });

                handle(DRAW_MOUSE_DRAGGED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            draw_mouse_dragged_msg e;
                            e.from_message(m);
                            _api->draw_mouse_dragged(e.id, e.button, e.x, e.y);
                        });

                handle(DRAW_MOUSE_MOVED, [&](const m::message& m)
                        {
                            if(!m::is_local(m)) return;
                            draw_mouse_moved_msg e;
                            e.from_message(m);
                            _api->draw_mouse_moved(e.id, e.x, e.y);
                        });
            }

            void backend_client::received_script_message(const m::message& m)
            {
                REQUIRE_EQUAL(m.meta.type, SCRIPT_MESSAGE);

                INVARIANT(_api);

                script_message sm{m, _api};
                _api->message_received(sm);
            }

            void backend_client::received_event_message(const m::message& m)
            {
                REQUIRE_EQUAL(m.meta.type, EVENT_MESSAGE);
                INVARIANT(_api);

                event_message em{m, _api};
                _api->event_received(em);
            }

            void backend_client::run(const std::string& code)
            {
                INVARIANT(mail());
                run_code_msg m;
                m.code = code;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::button_clicked(ref_id id)
            {
                INVARIANT(mail());
                button_clicked_msg m;
                m.id = id;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::edit_edited(ref_id id)
            {
                INVARIANT(mail());
                edit_edited_msg m;
                m.id = id;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::edit_finished(ref_id id)
            {
                INVARIANT(mail());
                edit_finished_msg m;
                m.id = id;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::text_edit_edited(ref_id id)
            {
                INVARIANT(mail());
                text_edit_edited_msg m;
                m.id = id;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::timer_triggered(ref_id id)
            {
                INVARIANT(mail());
                timer_triggered_msg m;
                m.id = id;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::got_sound(ref_id id, const util::bytes& d)
            {
                INVARIANT(mail());
                got_sound_msg m;
                m.id = id;
                m.data = d;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::draw_mouse_pressed(ref_id id, int button, int x, int y)
            {
                INVARIANT(mail());
                draw_mouse_pressed_msg m;
                m.id = id;
                m.button = button;
                m.x = x;
                m.y = y;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::draw_mouse_released(ref_id id, int button, int x, int y)
            {
                INVARIANT(mail());
                draw_mouse_released_msg m;
                m.id = id;
                m.button = button;
                m.x = x;
                m.y = y;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::draw_mouse_dragged(ref_id id, int button, int x, int y)
            {
                INVARIANT(mail());
                draw_mouse_dragged_msg m;
                m.id = id;
                m.button = button;
                m.x = x;
                m.y = y;
                mail()->push_inbox(m.to_message());
            }

            void backend_client::draw_mouse_moved(ref_id id, int x, int y)
            {
                INVARIANT(mail());
                draw_mouse_moved_msg m;
                m.id = id;
                m.x = x;
                m.y = y;
                mail()->push_inbox(m.to_message());
            }
        }
    }
}
