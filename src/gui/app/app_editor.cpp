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

#include <QtWidgets>

#include "gui/app/app_editor.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/serialize.hpp"
#include "util/string.hpp"
#include "util/uuid.hpp"

#include <QTimer>
#include <QSplitter>

#include <functional>
#include <sstream>
#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;
namespace l = fire::gui::lua;
namespace bf = boost::filesystem;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string APP_EDITOR = "APP_EDITOR";
            const std::string LUA_KEYWORDS = "\\b(store|app|and|break|do|else|elseif|end|false|for|function|if|in|local|nil|not|or|repeat|return|then|true|until|while|pairs)\\b";
            const std::string LUA_QUOTE = "\".*[^\\\\]\"";
            const std::string LUA_COMMENT = "--.*$";

            using api_doc = std::unordered_map<std::string, std::string>;
            const api_doc API_KEYWORDS{
                {"add", "widget:add(item) -- adds an item to the widget"},
                {"alert", "app:alert() -- alerts user that there was something of interest occurred"},
                {"append", "data:append(data) -- appends binary data at the end"},
                {"button", "app:button(text) -- creates a button"},
                {"dropdown", "app:dropdown() -- creates a dropdown"},
                {"bin_data", "app:bin_data() -- creates an empty binary data object"},
                {"callback","widget:callback() -- returns the primary callback set for the widget"},
                {"circle", "draw:circle(x,y,radius) -- draws a circle"},
                {"clear","widget:clear() -- clears the widget of data"},
                {"contact","app:contact(index) -- returns the contact at the index"},
                {"data","file:data() -- returns binary or ascii data of the file"},
                {"disable", "widget:disable() -- disables the widget"},
                {"draw", "app:draw(width, height) -- creates a drawing canvas"},
                {"edit","app:edit(text) -- creates an edit box"},
                {"vclock","app:vclock() -- makes a new vector clock/version vector"},
                {"edited_callback", "edit:edited_callback() -- returns the callback used when text is changed"},
                {"enable", "widget:enable() -- enables the widget"},
                {"enabled", "widget:enabled() -- returns true if the widget is enabled"},
                {"finished_callback","edit:finished_callback() -- returns the callback used when enter is pressed"},
                {"from", "message:from() -- returns the contact who sent the message"},
                {"from_str", "bin_data:from_str(str) -- converts a string to binary data"},
                {"has", "dict:has(key) -- returns true if the dictionary has a value with the key"},
                {"get", "object:get(item) -- returns an item from the object"},
                {"get_bin", "dict:get_bin -- returns binary data with the key from the dictionary"},
                {"get_vclock", "dict:get_vclock -- returns vclock with the key from the dictionary"},
                {"get_pen", "draw:get_pen() -- return the current pen being used"},
                {"good", "file:good() -- returns true if the file was read successfully"},
                {"grid", "app:grid() -- creates a grid layout which you can use to place widgets in a grid"},
                {"grow", "app:grow() -- grows the App vertically to fit all content"},
                {"height", "app:height(pixels) -- sets the height of the app"},
                {"width", "app:width(pixels) -- sets the width of the app"},
                {"hide", "widget:hide() -- hides the widget"},
                {"i_started", "app:i_started -- returns true if the user started the app, and false if it started remotely"},
                {"id", "contact:id() -- returns the id of the contact"},
                {"image", "app:image(data) -- creates an image from the data"},
                {"interval", "timer:interval(milliseconds) -- sets the timers interval"},
                {"is_local", "message:is_local() -- returns true if the message originated locally"},
                {"label","app:label(text) -- creates a label with the text specified"},
                {"last_contact", "app:last_contact() -- returns the index of the last contact"},
                {"line", "draw:line(x1, y1, x2, y2) -- draws a line"},
                {"list", "app:list() -- creates a list widget"},
                {"mic", "app:mic(callback, codec) -- creates an new microphone to get sound. Codecs (pcm, mp3)"},
                {"speaker", "app:speaker(codec) -- creates an new speaker for playing sound. Codecs (pcm,mp3)"},
                {"audio_encoder", "app:audio_encoder() -- creates an audio encoder that can encode pcm"},
                {"audio_decoder", "app:audio_decoder() -- creates an audio decoder that can decode to pcm"},
                {"message", "app:message() -- creates an new message"},
                {"mute", "speaker:mute() -- mutes the speaker"},
                {"unmute", "speaker:unmute() -- unmutes the speaker"},
                {"name", "contact:name() -- returns the name of the contact"},
                {"not_robust", "message:not_robust() -- this turns the message into one that won't resend if it fails. Use for data that is fine for losing"},
                {"online", "contact:online() -- returns true if the contact is online"},
                {"overlay", "bin_data:overlay(pos, bin_data) -- overlays the bin_data at the postiion"},
                {"open_bin_file", "app:open_bin_file() -- allows user to select a file to open and opens it in binary mode"},
                {"open_file", "app:open_file() -- allows user to select a file to open and opens it in text mode"},
                {"pen", "app:pen(color, width) -- creates a pen of the color and width specified"},
                {"place","app:place(widget, row, column) -- places the widget in the spot specified"},
                {"place_across", "app:place_across(widget, row, column, rows, columns) -- place the widget in the spot specified, across several rows and columns"},
                {"print", "app:print(text) -- prints the text to the App Editor output or log"},
                {"remove", "dict:remove(key) -- removes data with the key"},
                {"running", "timer:running() -- returns true if the timer is running"},
                {"save_bin_file", "app:save_bin_file(name, data) -- allows the user to select a binary file to save to"},
                {"save_file", "app:save_file(name, text) -- allows the user to select a file to save to"},
                {"self", "app:self() -- returns the user information"},
                {"send", "app:send(message) -- sends the message to everyone connected to the App"},
                {"send_local", "app:send_local(message) -- sends the message locally to all Apps in the conversation"},
                {"send_to", "app:send_to(contact, message) -- sends the message to the contact specified"},
                {"set", "dict:set(key, value) -- stores the value with the key. The value can be any lua type"},
                {"selected", "dropdown:selected() -- returns the index of the selected item"},
                {"set_type", "message:set_type(value) -- sets the type of message"},
                {"set_bin", "dict:set_bin(key, data) -- stores binary data with the key"},
                {"set_vclock", "dict:set_vclock(key, vclock) -- stores vclock data with the key"},
                {"set_image", "button:set_image(image) -- sets an image for the button"},
                {"set_text", "widget:set_text(text) -- sets the widget's text"},
                {"set_name", "widget:set_name(text) -- sets the widget's name turning on remote events"},
                {"set_pen", "shape:set_pen(pen) -- sets the pen of the drawn shape"},
                {"set_style", "widget:set_style(style) -- sets the style of the widget. It is a Qt style sheet."},
                {"show", "widget:show() -- shows the widget"},
                {"size", "widget:size() -- returns the size of the widget"},
                {"start", "timer:start() -- starts the timer"},
                {"stop", "timer:stop() -- stops the timer"},
                {"str", "data:str() -- converts the binary data to a string"},
                {"sub", "data:sub(index, size) -- returns subset of the data"},
                {"text", "widget:text() -- returns the text of the widget"},
                {"text_edit", "app:text_edit(text) -- creates a multi-line text edit"},
                {"timer", "app:timer(milliseconds, callback) -- creates a timer that will execute the callback specified"},
                {"type", "message:type() -- returns the type of message"},
                {"total_contacts", "app:total_contacts() -- returns the count of contacts connected to the app"},
                {"visible", "widget:visible() -- returns true if the widget is visible"},
                {"when_clicked", "button:when_clicked(code) -- will execute the code when the button is clicked"},
                {"when_selected", "dropdown:when_selected(callback) -- will execute the callback when an item from the dropdown is selected"},
                {"when_edited", "edit:when_edited(callback) -- will call the callback when text is edited"},
                {"when_finished", "edit:when_finished(callback) -- will call the callback when return is pressed"},
                {"when_sound", "mic:when_sound(callback) -- will call the callback when sound comes from the mic"},
                {"when_local_message_received", "app:when_local_message_received(callback) -- will call the callback when a local message is received"},
                {"when_local_message", "app:when_local_message(type, callback) -- will call the callback when a local message is received of the type specified"},
                {"when_message_received", "app:when_message_received(callback) -- will call the callback when a message is received"},
                {"when_message", "app:when_message(type,callback) -- will call the callback when a message is received of the type specified"},
                {"when_mouse_dragged", "draw:when_mouse_dragged(callback) -- will call the callback when the mouse is dragged"},
                {"when_mouse_moved", "draw:when_mouse_moved(callback) -- will call the callback when the mouse is moved"},
                {"when_mouse_pressed", "draw:when_mouse_pressed(callback) -- will call the callback when a mouse button is pressed"},
                {"when_mouse_released", "draw:when_mouse_released(callback) -- will call the callback when a mouse button is released"},
                {"when_joined", "app:when_joined(callback) -- will call the callback when a contact joins the conversation"},
                {"when_quit", "app:when_quit(callback) -- will call the callback when a contact quits the conversation"},
                {"when_triggered", "timer:when_triggered(callback) -- will call the callback when the timer fires"},
                {"who_started", "app:who_started() -- returns the contact who started the app"},
                {"encode", "audio_encoder:encode(pcm data) -- recieves mono 12khz pcm and encodes it using opus"},
                {"decode", "audio_decoder:decode(opus data) -- recieves mono 12khz opus and decodes it to pcm"},
                {"inc","vclock:inc() -- increment vclock"},
                {"merge","vclock:merge(vclock) -- merge two vclock"},
                {"conflict","vclock:conflict(vclock) -- returns true if there is a conflict between two clocks"},
                {"concurrent","vclock:concurrent(vclock) -- returns true if two clocks are concurrent"},
                {"comp","vclock:comp(vclock) -- compares two clocks. -1 if less, 0 if concurrent, 1 if new"},
                {"equals","vclock::equals(vclock) -- returns true if two vclock are identical"},
            };

            const std::string LUA_NUMBERS = "[0-9\\.]+";
            const std::string LUA_OPERATORS = "[=+-\\*\\^:%#~<>\\(\\){}\\[\\];:,]+";

            const std::string EXAMPLE_APP =
                "-- Write your program here.\n"
                "-- As you write, the program will run\n"
                "-- and the results will show on the right.\n"
                "\n"
                "-- Create a button and place it on the grid.\n"
                "hello = app:button('Hello')\n"
                "app:place(hello, 0, 0)\n"
                "\n"
                "--choose which function to run when the\n" 
                "--button is clicked.\n"
                "hello:when_clicked(\"hello_world()\")\n"
                "\n"
                "--Set the button text to 'Hello World'.\n"
                "function hello_world()\n"
                "    hello:set_text(\"Hello World\")\n"
                "end\n";

            namespace
            {
                const std::string ERROR_SELECTION = "error";
                const char SELECTION_ALPHA = 75;
                const char CURSOR_ALPHA = 100;
                const char DARKER = 60;
                const QColor ERROR_COLOR{255, 0, 0, SELECTION_ALPHA};

                const size_t TIMER_UPDATE = 1000; //in milliseconds
                const size_t PADDING = 20;
                const int MIN_EDIT_HEIGHT = 320;
                const int MIN_EDIT_WIDTH = 320;
                const int MIN_APP_WIDTH = 240;
                const std::string SCRIPT_CODE_MESSAGE = "scpt";
                const std::string SCRIPT_INIT_MESSAGE = "init";
                const std::string CURSOR_POS_MESSAGE = "cp";

                const std::vector<std::string> CURSOR_COLORS = {
                    "#AEC6CF",
                    "#CB99C9",
                    "#836953",
                    "#B19CD9",
                    "#DEA5A4",
                    "#C23B22",
                    "#F49AC2",
                    "#FF6961",
                    "#FDFD96",
                    "#77DD77",
                    "#FFB347",
                    "#779ECB",
                    "#966FD6",
                    "#03C03C",
                    "#CFCFC4",
                    "#B39EB5",
                    "#FFD1DC",
                };
            }

            f_message(text_script)
            {
                bool init = false;
                u::cr_string code;
                u::bytes data;

                f_message_init(text_script, SCRIPT_CODE_MESSAGE);
                f_serialize_in
                {
                    if(f_has("i")) f_sk("i", init);

                    std::string c;
                    f_sk("c", c);

                    u::dict ld;
                    f_sk("l", ld);
                    auto l = u::to_tracked_sclock(ld);

                    code = u::cr_string{l, c};
                    f_sk("d", data);
                }

                f_serialize_out
                {
                    if(init) f_sk("i", init);
                    f_sk("c", code.str());
                    f_sk("l", u::to_dict(code.clock()));
                    f_sk("d", data);
                }
            };

            f_message(script_init)
            {
                f_message_init(script_init, SCRIPT_INIT_MESSAGE);
                f_serialize_empty;
            };

            f_message(cursor_pos)
            {
                int s = 0;     //start
                int e = 0;     //end
                std::string c; //color
                f_message_init(cursor_pos, CURSOR_POS_MESSAGE);
                f_serialize 
                { 
                    f_s(s);
                    f_s(e);
                    f_s(c);
                };
            };

            app_editor::app_editor(
                    app_service_ptr app_service, 
                    app_reaper_ptr app_reaper, 
                    s::conversation_service_ptr conversation_s, 
                    s::conversation_ptr conversation, 
                    app_ptr app) :
                generic_app{},
                _from_id(conversation->user_service()->user().info().id()),
                _id(u::uuid()),
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _code{conversation->user_service()->user().info().id()},
                _app{app},
                _run_state{READY}
            {
                REQUIRE(app_service);
                REQUIRE(app_reaper);
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);

                init();

                INVARIANT(_app);
                ENSURE(_conversation_service);
                ENSURE(_conversation);
                ENSURE(_app_service);
                ENSURE(_app_reaper);
            }

            app_editor::app_editor(
                    const std::string& from_id, 
                    const std::string& id, 
                    app_service_ptr app_service, 
                    app_reaper_ptr app_reaper, 
                    s::conversation_service_ptr conversation_s, 
                    s::conversation_ptr conversation,
                    app_ptr app) :
                generic_app{},
                _from_id(from_id),
                _id(id),
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_s},
                _conversation{conversation},
                _code{conversation->user_service()->user().info().id()},
                _app{app},
                _run_state{READY}
                
            {
                REQUIRE(app_service);
                REQUIRE(app_reaper);
                REQUIRE(conversation_s);
                REQUIRE(conversation);
                REQUIRE(app);

                init();

                INVARIANT(_app);
                ENSURE(_conversation_service);
                ENSURE(_conversation);
                ENSURE(_app_service);
                ENSURE(_app_reaper);
            }

            app_editor::~app_editor()
            {
                INVARIANT(_conversation);
                INVARIANT(_front);
                INVARIANT(_back);
                INVARIANT(_app);
                INVARIANT(_app_reaper);

                LOG << "closing app " << _app->name() << "(" << _app->id() << ")" << std::endl;
                closed_app c;
                c.name = _app->name();
                c.id = _app->id();
                c.front = _front;
                c.back = _back;
                _app_reaper->reap(c);
            }

            void app_editor::init()
            {
                INVARIANT(_conversation);
                INVARIANT(_conversation->user_service());

                set_title("App Editor");

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation->user_service(), _mail);
                _code.init_set(_app->code());

                init_cursor_color();

                ENSURE(_sender);
                ENSURE(_mail);
            }

            void app_editor::start()
            {
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_conversation->user_service());
                INVARIANT(_app_service);
                INVARIANT(_app_reaper);
                INVARIANT(_app);

                //create gui
                auto tabs = new QTabWidget{this};
                tabs->setTabPosition(QTabWidget::West);
                tabs->setDocumentMode(true);
                layout()->addWidget(tabs, 1,0,2,3);

                set_main(tabs);

                //code tab
                _code_tab = new QWidget{this};
                auto code_layout = new QGridLayout{_code_tab};
                tabs->addTab(_code_tab, tr("code"));

                init_code_tab(code_layout);

                //data tab
                _data_tab = new QWidget{this};
                auto data_layout = new QGridLayout{_data_tab};
                tabs->addTab(_data_tab, tr("data"));

                init_data_tab(data_layout);

                //init handlers
                init_handlers();

                //run app
                run_script();
                adjust_size();

                INVARIANT(_app);
                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
                INVARIANT(_status);
                INVARIANT(_code_tab);
                INVARIANT(_data_tab);
            }

            void app_editor::init_code_tab(QGridLayout* l)
            {
                REQUIRE(l);
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_mail);


                //add status bar
                _status = new QLabel;
                l->addWidget(_status, 0, 0, 1, 2);

                //save button
                _save_button = new QPushButton;
                make_save(*_save_button);
                _save_button->setToolTip(tr("Save"));
                l->addWidget(_save_button, 0, 2);
                connect(_save_button, SIGNAL(clicked()), this, SLOT(save_app()));

                //export button
                auto expt = new QPushButton;
                make_export(*expt);
                l->addWidget(expt, 0, 3);
                expt->setToolTip(tr("Export to File"));
                connect(expt, SIGNAL(clicked()), this, SLOT(export_app()));

                //mic button
                _mic = new QPushButton;
                make_big_mic(*_mic);
                _mic->setVisible(false);
                connect(_mic, SIGNAL(clicked()), this, SLOT(toggle_mic()));
                l->addWidget(_mic, 0,1);

                CHECK_FALSE(_resource.mic);
                update_mic_widget();

                //add application canvas and print area
                _canvas = new QWidget;
                _canvas_layout = new QGridLayout;
                _canvas->setLayout(_canvas_layout);
                _canvas->setMinimumWidth(MIN_APP_WIDTH);
                _output = new list;

                auto front = std::make_shared<qtw::qt_frontend>(_canvas, _canvas_layout, _output);
                connect(front.get(), SIGNAL(do_adjust_size()), this, SLOT(got_adjust_size()));
                connect(front.get(), SIGNAL(mic_added()), this, SLOT(got_mic_added()));

                _front = std::make_shared<qtw::qt_frontend_client>(front);
                connect(_front.get(), SIGNAL(got_report_error(const std::string&)), this, SLOT(update_error(const std::string&)));

                _api = std::make_shared<l::lua_api>(
                        _app, 
                        _sender, 
                        _conversation, 
                        _conversation_service, 
                        _front.get());

                _back = std::make_shared<l::backend_client>(_api, _mail); 

                _front->set_backend(_back.get());

                //text edit
                _script = new app_text_editor{_api.get()};
                _script->setMinimumSize(QSize{MIN_EDIT_WIDTH, MIN_EDIT_HEIGHT});
                _script->setWordWrapMode(QTextOption::NoWrap);
                _script->setTabStopDistance(40);
                _highlighter = new lua_highlighter(_script->document());

                _started = _app->code().empty() ? start_state::GET_CODE : start_state::DONE_START;
                _script->setPlainText(_app->code().c_str());
                connect(_script, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(text_typed(QKeyEvent*)));
                connect(_script, SIGNAL(cursorPositionChanged()), this, SLOT(cursor_changed()));

                setMinimumHeight(layout()->sizeHint().height() + PADDING);

                //setup vertical splitter
                auto vs = new QSplitter{Qt::Horizontal};
                vs->setFrameShape(QFrame::NoFrame);
                vs->setContentsMargins(0,0,0,0);
                vs->addWidget(_script);
                vs->addWidget(_canvas);

                //setup horizontal splitter
                auto hs = new QSplitter{Qt::Vertical};
                hs->setFrameShape(QFrame::NoFrame);
                hs->setContentsMargins(0,0,0,0);

                hs->addWidget(vs);
                hs->addWidget(_output);

                hs->setStretchFactor(0, 1);
                hs->setStretchFactor(1, 0);

                l->addWidget(hs, 1, 0, 1, 4);

                //setup update timer
                auto *t2 = new QTimer(this);
                connect(t2, SIGNAL(timeout()), this, SLOT(update()));
                t2->start(TIMER_UPDATE);

                INVARIANT(_front);
                INVARIANT(_api);
                INVARIANT(_app);
                INVARIANT(_conversation);
                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_canvas);
                INVARIANT(_canvas_layout);
                INVARIANT(_output);
                INVARIANT(_status);
                INVARIANT(_save_button);
            }

            void app_editor::init_data()
            {
                INVARIANT(_data_items);
                INVARIANT(_app);

                _data_items->clear();

                for(const auto& p : _app->data())
                {
                    auto di = new data_item(_app->data(), p.first);
                    connect(di, SIGNAL(key_was_clicked(QString)), this, SLOT(key_was_clicked(QString)));
                    connect(di, SIGNAL(data_updated()), this, SLOT(data_updated()));

                    _data_items->add(di);
                }

                ENSURE_EQUAL(_data_items->size(), _app->data().size());
            }

            void app_editor::init_data_tab(QGridLayout* l)
            {
                REQUIRE(l);
                INVARIANT(root());
                INVARIANT(layout());
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_app);

                auto key_label = new QLabel{tr("key:")};
                auto value_label = new QLabel{tr("value:")};
                _data_key = new QLineEdit;
                connect(_data_key, SIGNAL(textChanged(QString)), this, SLOT(data_key_edited()));

                auto value_widget = new QWidget;
                auto value_layout = new QHBoxLayout{value_widget};
                _data_value = new QLineEdit;
                connect(_data_value, SIGNAL(returnPressed()), this, SLOT(add_data()));

                auto load_button = new QPushButton{tr("...")};
                load_button->setMaximumSize(20,20);
                connect(load_button, SIGNAL(clicked()), this, SLOT(load_data_from_file()));

                value_layout->addWidget(_data_value);
                value_layout->addWidget(load_button);

                _add_button = new QPushButton;
                make_plus(*_add_button);
                connect(_add_button, SIGNAL(clicked()), this, SLOT(add_data()));

                l->addWidget(key_label, 0, 0);
                l->addWidget(_data_key, 0, 1);
                l->addWidget(value_label, 0, 2);
                l->addWidget(value_widget, 0, 3);
                l->addWidget(_add_button, 0, 4);

                _data_items = new list;
                l->addWidget(_data_items, 1, 0, 5, 0);

                init_data();
                data_key_edited();

                ENSURE(_data_key);
                ENSURE(_data_value);
                ENSURE(_add_button);
                ENSURE(_data_items);
            }

            void app_editor::init_cursor_color()
            {
                INVARIANT(_conversation);
                INVARIANT(_conversation->user_service());

                //compute color based on hash of user id.
                const auto id = _conversation->user_service()->user().info().id();
                std::hash<std::string> hash_func;
                auto color_index = hash_func(id) % CURSOR_COLORS.size();
                CHECK_RANGE(color_index, 0, CURSOR_COLORS.size());

                //get color
                auto rgb = CURSOR_COLORS[color_index];
                _cursor_color = QColor(rgb.c_str());
            }

            void app_editor::data_key_edited()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                bool enabled = !_data_key->text().isEmpty();
                _data_value->setEnabled(enabled);
                _add_button->setEnabled(enabled);
            }

            void app_editor::key_was_clicked(QString key)
            {
                INVARIANT(_data_key);
                _data_key->setText(key);
            }

            void app_editor::add_data()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                INVARIANT(_app);
                auto k = gui::convert(_data_key->text());
                u::trim(k);
                auto vt = gui::convert(_data_value->text());
                u::trim(vt);
                u::value v = vt;

                //must have a key
                if(k.empty()) return;

                //silly check to see loaded a file or set text value
                if(vt == "<file>") _app->data().set(k, _data_bytes);
                else 
                {
                    //try to convert to number, if not a number then keep string.
                    try { v = boost::lexical_cast<double>(vt); }catch(...){}

                    _app->data().set(k, v);
                }

                //refresh ui
                init_data();

                //clear input
                _data_key->clear();
                _data_value->clear();
                _data_bytes.clear();
                _data_key->setFocus(Qt::OtherFocusReason);

                data_updated();
            }

            void app_editor::contact_quit(const std::string& id)
            {
                REQUIRE_FALSE(id.empty());
                INVARIANT(_back);

                _back->contact_quit(id);
            }

            void app_editor::data_updated()
            {
                _run_state = CODE_CHANGED;
                _code.clock()++;
                send_script(true);
            }

            void app_editor::load_data_from_file()
            {
                INVARIANT(_data_key);
                INVARIANT(_data_value);
                auto sf = get_file_name(this);
                if(sf.empty()) return;

                if(!load_raw_from_file(sf, _data_bytes)) return;

                _data_value->setText("<file>");
                if(_data_key->text().isEmpty())
                {
                    bf::path p = sf;
                    _data_key->setText(p.filename().string().c_str());
                }
            }

            const std::string& app_editor::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& app_editor::type() const
            {
                ENSURE_FALSE(APP_EDITOR.empty());
                return APP_EDITOR;
            }

            m::mailbox_ptr app_editor::mail()
            {
                ENSURE(_mail);
                return _mail;
            }



            app_text_editor::app_text_editor(lua::lua_api* api) : _api{api}
            {
                REQUIRE(api);

                _c = new QCompleter;
                _c->setCompletionColumn(1);
                _c->setModel(new QStandardItemModel{this});
                _c->setWidget(this);
                _c->setCompletionMode(QCompleter::PopupCompletion);
                _c->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
                _c->setCaseSensitivity(Qt::CaseInsensitive);
                _c->setWrapAround(false);
                connect(_c, SIGNAL(activated(QString)), this, SLOT(insert_completion(QString)));

                REQUIRE(_api);
                REQUIRE(_c);
            }

            QStringList app_text_editor::auto_complete_list(const std::string& obj)
            {
                INVARIANT(_api);
                INVARIANT(_api->state);
                QStringList r;

                //auto complete list based on global object methods
                if(!obj.empty())
                    for(const auto& k : _api->state->getKeys(obj.c_str()))
                        r << k.c_str(); 

                //if it is empty, then auto complete list is all api keywords
                if(r.isEmpty())
                    for(const auto& kw : API_KEYWORDS)
                        r << kw.first.c_str();

                return r;
            }

            void app_text_editor::set_auto_complete_model(const QStringList& l)
            {
                INVARIANT(_c);

                auto model = dynamic_cast<QStandardItemModel*>(_c->model());
                CHECK(model);
                model->setColumnCount(2);
                model->setRowCount(l.count());
                int r = 0;
                for(const auto& i : l)
                {
                    auto v = API_KEYWORDS.find(gui::convert(i));
                    if(v == API_KEYWORDS.end()) continue;
                    model->setItem(r, 0, new QStandardItem{v->second.c_str()});
                    model->setItem(r, 1, new QStandardItem{v->first.c_str()});
                    r++;
                }
            }

            QString app_text_editor::object_left_of_cursor() const
            {
                auto c = textCursor();
                c.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
                if(c.selectedText() == ":" )
                {
                    c.setPosition(c.position());
                    c.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
                } 
                else
                {
                    c.movePosition(QTextCursor::WordLeft);
                    c.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
                }

                return c.selectedText();
            }

            QString app_text_editor::char_left_of_word() const
            {
                auto c = textCursor();
                c.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
                if(c.selectedText() == ":" ) return ":";
                c.setPosition(c.position());
                c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
                return c.selectedText();
            }

            QString app_text_editor::word_under_cursor() const
            {
                auto c = textCursor();
                c.select(QTextCursor::WordUnderCursor);
                return c.selectedText();
            }

            void app_text_editor::insert_completion(const QString& t)
            {
                INVARIANT(_c);
                auto c = textCursor();
                
                //figure out diff, move to end of current word and insert remainder
                auto diff = t.length() - _c->completionPrefix().length();
                c.movePosition(QTextCursor::Left);
                c.movePosition(QTextCursor::EndOfWord);
                c.insertText(t.right(diff));

                setTextCursor(c);

                emit keyPressed(nullptr);
            }

            //This code was borrowed from the QT TextEdit custom completer example
            //http://qt-project.org/doc/qt-4.8/tools-customcompleter.html
            void app_text_editor::keyPressEvent(QKeyEvent* e)
            {
                INVARIANT(_c);
                if(!e) return;
                if (_c->popup()->isVisible()) 
                {
                    //let popup handle enter, tab, return, etc
                    switch (e->key()) 
                    {
                        case Qt::Key_Enter:
                        case Qt::Key_Return:
                        case Qt::Key_Escape:
                        case Qt::Key_Tab:
                        case Qt::Key_Backtab: e->ignore(); return; 
                        default: break;
                    }
                }

                bool shortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E);
                bool ctr_shift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
                bool emit_e = false;

                if(!shortcut) 
                {
                    QTextEdit::keyPressEvent(e);
                    emit_e = true;
                }

                if(ctr_shift && e->text().isEmpty()) 
                {
                    if(emit_e) emit keyPressed(e);
                    return;
                }

                static const QString EOW{"~!@#$%^&*()+{}|:\"<>?,./;'[]\\-="};
                bool modifier = (e->modifiers() != Qt::NoModifier) && !ctr_shift;
                auto left_char = char_left_of_word();
                auto object = object_left_of_cursor();
                auto prefix = word_under_cursor();
                bool colon = left_char == ":";

                set_auto_complete_model(auto_complete_list(gui::convert(object)));

                //hide popup if
                // 1. is not shortcut and
                // 2. left char of word is not a colon or 
                // 3. modifier or
                // 4. char is EOW character
                if (!shortcut && (
                            !colon
                            || modifier 
                            || (!prefix.isEmpty() && EOW.contains(e->text().right(1))))) 
                {
                    _c->popup()->hide();
                    if(emit_e) emit keyPressed(e);
                    return;
                }

                if (prefix != _c->completionPrefix()) 
                {
                    _c->setCompletionPrefix(prefix);
                    _c->popup()->setCurrentIndex(_c->completionModel()->index(0, 0));
                }

                QRect cr = cursorRect();
                cr.setWidth(_c->popup()->sizeHintForColumn(0)
                        + _c->popup()->verticalScrollBar()->sizeHint().width());

                _c->complete(cr); 
                    
                if(emit_e) emit keyPressed(e);
            }

            void app_editor::cursor_changed()
            {
                INVARIANT(_script);

                auto cursor = _script->textCursor();
                cursor_pos p;
                p.s = cursor.selectionStart();
                p.e = cursor.selectionEnd();

                //don't send at 0,0
                if(p.s == 0 && p.e == 0) return;

                p.c = 
                { 
                    static_cast<char>(_cursor_color.red()), 
                    static_cast<char>(_cursor_color.green()), 
                    static_cast<char>(_cursor_color.blue())
                };

                send_all(p.to_message());
            }

            void app_editor::text_typed(QKeyEvent* e)
            {
                INVARIANT(_script);

                //update code before send
                _code.set(gui::convert(_script->toPlainText()));
                _dirty = true;

                send_script(false);
            }

            bool app_editor::prepare_script_message(text_script& tm, bool send_data)
            {
                INVARIANT(_script);
                INVARIANT(_api);

                //export the data
                if(send_data)
                {
                    u::dict data;
                    _app->data().export_to(data);
                    tm.data = u::encode(data);
                }
                return true;
            }

            void app_editor::send_all(const m::message& m)
            {
                INVARIANT(_conversation);
                INVARIANT(_sender);

                for(auto c : _conversation->contacts().list())
                {
                    CHECK(c);
                    _sender->send(c->id(), m); 
                }
            }

            void app_editor::send_script(bool send_data)
            {
                INVARIANT(_script);
                INVARIANT(_conversation);
                INVARIANT(_api);

                //set the code
                text_script tm;
                tm.code = _code;
                if(!prepare_script_message(tm, send_data)) return;

                //send it all
                auto m = tm.to_message();
                send_all(m);
            }

            void app_editor::send_script_to(const std::string& id)
            {
                text_script tm;
                tm.code = _code;
                if(!prepare_script_message(tm, true)) return;

                _sender->send(id, tm.to_message()); 
            }

            void app_editor::ask_for_script()
            {
                INVARIANT(_sender);
                INVARIANT(_conversation);
                INVARIANT_FALSE(_from_id.empty());

                //if from self, return
                if(_from_id == _conversation->user_service()->user().info().id()) return;
                script_init init;
                _sender->send(_from_id, init.to_message());
            }

            void app_editor::run_script()
            {
                INVARIANT(_script);
                INVARIANT(_conversation);
                INVARIANT(_conversation_service);
                INVARIANT(_front);
                INVARIANT(_back);
                INVARIANT(_output);

                //alert of change
                _conversation_service->fire_conversation_alert(_conversation->id(), visible());
                alerted();

                //reset ui
                update_status_to_no_errors();
                _back->reset();
                _front->reset();

                //run the code
                if(!_code.str().empty()) _back->run(_code.str());

                //update ui
                init_data();
            }

            void app_editor::set_selection(const std::string& key, int start, int end, const QColor& color)
            {
                INVARIANT(_script);
                
                selection s;
                s.start = start;
                s.end = end;
                s.color = color;

                _selections[key] = s;

                update_selections();
            }

            void app_editor::set_selection(const std::string& key, const QTextCursor& c, const QColor& color)
            {
                set_selection(key, c.selectionStart(), c.selectionEnd() , color);
            }

            QTextEdit::ExtraSelection make_extra_selection(const selection& s, QTextCursor c)
            {
                c.setPosition(s.start);
                c.setPosition(s.end, QTextCursor::KeepAnchor);

                //setup color
                QBrush b{s.color};

                //create selection
                QTextEdit::ExtraSelection h;
                h.cursor = c;
                h.format.setBackground(b);
                h.format.setProperty(QTextFormat::FullWidthSelection, true);

                return h;
            }

            QColor create_cursor_color(QColor c)
            {
                c.setAlpha(CURSOR_ALPHA);
                c.setRed(std::max(0, c.red() - DARKER));
                c.setGreen(std::max(0, c.green() - DARKER));
                c.setBlue(std::max(0, c.blue() - DARKER));

                return c;
            }

            void app_editor::update_selections()
            {
                INVARIANT(_script);

                QList<QTextEdit::ExtraSelection> extras;
                for( const auto& pr : _selections) 
                {
                    const auto& s = pr.second;

                    //create selection
                    auto c = _script->textCursor();
                    extras << make_extra_selection(s, c);

                    //if start and end are the same, overlay another extra
                    //with same color to show cursor.
                    if(s.start == s.end) 
                    {
                        selection cp = s;
                        cp.end += 1;
                        cp.color = create_cursor_color(cp.color);
                        extras << make_extra_selection(cp, c);
                    }
                }
                _script->setExtraSelections( extras );
            }

            void app_editor::clear_selection(const std::string& key) 
            {
                _selections.erase(key);

                update_selections();
            }
            
            void app_editor::update_error(const std::string& msg)
            {
                INVARIANT(_script);
                INVARIANT(_api);

                auto e = _api->get_error();

                QList<QTextEdit::ExtraSelection> extras;
                if(e.line != -1)
                {
                    int line = e.line - 2;
                    if(line >= 0) 
                    {
                        auto c = _script->textCursor();
                        c.movePosition(QTextCursor::Start);
                        c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line); 
                        set_selection(ERROR_SELECTION, c, ERROR_COLOR);
                    }
                    else clear_selection(ERROR_SELECTION);
                    update_status_to_errors();
                }
            }

            void app_editor::got_adjust_size()
            {
                adjust_size();
            }

            void app_editor::got_mic_added()
            {
                INVARIANT(_mic);
                _mic->setVisible(true);
            }

            bool app_editor::set_app_name()
            {
                INVARIANT(_app);
                if(_app->name().empty())
                {
                    bool ok = false;
                    std::string name = "";

                    QString r = QInputDialog::getText(
                            0, 
                            tr("Name Your App"),
                            tr("App Name"),
                            QLineEdit::Normal, name.c_str(), &ok);

                    if(ok && !r.isEmpty()) name = gui::convert(r);
                    else return false;

                    _app->name(name);
                }
                return true;
            }

            void app_editor::update_app_code()
            {
                INVARIANT(_app);
                INVARIANT(_script);

                _app->code(_code.str());
            }

            void app_editor::save_app() 
            try
            {
                INVARIANT(_conversation);
                INVARIANT(_app_service);
                INVARIANT(_app);
                if(!set_app_name()) return;

                update_app_code();

                _app_service->save_app(*_app);
                _dirty = false;
                update_save_button();
            }
            catch(std::runtime_error& e)
            {
                std::stringstream ss;
                ss << "There was an error saving the app. " << e.what();
                QMessageBox::critical(this, tr("Error Saving App"), ss.str().c_str());
            }
            catch(...)
            {
                std::stringstream ss;
                ss << "There was an unexpected error saving the app.";
                QMessageBox::critical(this, tr("Error Saving App"), ss.str().c_str());
            }

            void app_editor::export_app() 
            try
            {
                INVARIANT(_conversation);
                INVARIANT(_app_service);

                set_app_name();
                update_app_code();

                std::string default_file = _app->name() + ".fab";

                auto file = QFileDialog::getSaveFileName(this, tr("Export App"),
                        default_file.c_str(),
                        tr("App (*.fab)"));

                if(file.isEmpty())
                    return;

                _app_service->export_app(*_app, gui::convert(file));
            }
            catch(std::runtime_error& e)
            {
                std::stringstream ss;
                ss << "There was an error exporting the app. " << e.what();
                QMessageBox::critical(this, tr("Error Exporting App"), ss.str().c_str());
            }
            catch(...)
            {
                std::stringstream ss;
                ss << "There was an unexpected error exporting the app.";
                QMessageBox::critical(this, tr("Error Exporting App"), ss.str().c_str());
            }

            void app_editor::update_status_to_errors()
            {
                INVARIANT(_status);
                make_error(*_status);
            }

            void app_editor::update_status_to_no_errors()
            {
                INVARIANT(_status);
                clear_selection(ERROR_SELECTION);
                make_thumbs_up(*_status);
            }

            void app_editor::update_status_to_typing()
            {
                INVARIANT(_status);
                make_progress_1(*_status);
            }

            void app_editor::update_status_to_waiting()
            {
                INVARIANT(_status);
                make_progress_2(*_status);
            }

            void app_editor::update_status_to_running()
            {
                INVARIANT(_status);
                make_progress_3(*_status);
            }

            void app_editor::update_save_button()
            {
                INVARIANT(_save_button);
                if(_dirty) make_green(*_save_button);
                else make_black(*_save_button);
            }

            void app_editor::init_update()
            {
                //ask for script first time on startup
                if(_started != start_state::GET_CODE) return;

                ask_for_script();
                _started = start_state::DONE_START;
            }

            void app_editor::update()
            {
                INVARIANT(_script);
                INVARIANT(_api);

                init_update();

                auto code = _code.str();
                int pos = _script->textCursor().position();

                switch(_run_state)
                {
                    case READY:
                        {
                            if(code != _prev_code) 
                            {
                                update_status_to_typing();
                                _run_state = CODE_CHANGED;
                            }
                            break;
                        }
                    case CODE_CHANGED:
                        {
                            if(pos == _prev_pos && code == _prev_code) 
                            {
                                update_status_to_waiting();
                                _run_state = DONE_TYPING;
                            }
                            break;
                        }
                    case DONE_TYPING:
                        {
                            if(pos == _prev_pos && code == _prev_code) 
                            {
                                update_status_to_running();
                                update_save_button();

                                //update status bar
                                run_script();
                                _run_state = READY;
                            } 
                            else
                            {
                                if(code != _prev_code) 
                                {
                                    update_status_to_typing();
                                    _run_state = CODE_CHANGED;
                                }
                                else
                                {
                                    update_status_to_waiting();
                                    _run_state = DONE_TYPING;
                                }
                            }

                            break;
                        }
                }

                _prev_code = code;
                _prev_pos = pos;
            }

            void app_editor::init_handlers() 
            {
                qRegisterMetaType<m::message>("fire::message::message");
                connect(this, SIGNAL(got_code(const fire::message::message&)), this, SLOT(received_code(const fire::message::message&)));
                connect(this, SIGNAL(got_init(const fire::message::message&)), this, SLOT(received_script_init(const fire::message::message&)));
                connect(this, SIGNAL(got_cursor_pos(const fire::message::message&)), this, SLOT(received_cursor_pos(const fire::message::message&)));

                INVARIANT(_back);
                using std::bind;
                using namespace std::placeholders;

                _back->handle(SCRIPT_CODE_MESSAGE, 
                        bind(&app_editor::emit_got_code, this, _1));

                _back->handle(SCRIPT_INIT_MESSAGE, 
                        bind(&app_editor::emit_got_init, this, _1));

                _back->handle(CURSOR_POS_MESSAGE, 
                        bind(&app_editor::emit_got_cursor_pos, this, _1));

                _back->start();
            }

            void app_editor::emit_got_code(const m::message& m)
            {
                emit got_code(m);
            }

            void app_editor::emit_got_init(const m::message& m)
            {
                emit got_init(m);
            }

            void app_editor::emit_got_cursor_pos(const m::message& m)
            {
                emit got_cursor_pos(m);
            }

            void app_editor::received_code(const m::message& m) 
            {
                REQUIRE_EQUAL(m.meta.type, SCRIPT_CODE_MESSAGE);
                INVARIANT(_conversation);
                INVARIANT(_app);

                text_script t;
                t.from_message(m);

                //check to see if contact is still in conversation.
                if(!_conversation->contacts().has(t.from_id)) return;

                //merge code if there is a conflict
                auto merged = _code.merge(t.code);
                _dirty = true;

                //update text
                auto pos = _script->textCursor().position();
                _script->setText(_code.str().c_str());

                //put cursor back
                auto cursor = _script->textCursor();
                cursor.setPosition(pos);
                _script->setTextCursor(cursor);

                update_selections();

                //update data
                bool data_changed = !t.data.empty();
                if(data_changed)
                {
                    u::dict data = u::decode<u::dict>(t.data);
                    if(merged == u::merge_result::UPDATED) 
                        _app->data().clear();
                    _app->data().import_from(data);

                    //update data ui
                    init_data();

                    _prev_code = _code.str();
                    _run_state = READY;
                    run_script();
                }
            }

            void app_editor::received_script_init(const m::message& m) 
            {
                REQUIRE_EQUAL(m.meta.type, SCRIPT_INIT_MESSAGE);
                INVARIANT(_conversation);

                script_init i;
                i.from_message(m);

                if(!_conversation->contacts().has(i.from_id)) return;

                send_script_to(i.from_id);
            }
            
            void app_editor::received_cursor_pos(const m::message& m) 
            {
                REQUIRE_EQUAL(m.meta.type, CURSOR_POS_MESSAGE);
                INVARIANT(_conversation);

                cursor_pos p;
                p.from_message(m);

                if(!_conversation->contacts().has(p.from_id)) return;
                if(p.c.size() != 3) return;

                //since we are packing rgb in a byte array, convert to unsigned to get back color.
                const unsigned char r = p.c[0], g = p.c[1], b = p.c[2];
                set_selection(p.from_id, p.s, p.e, QColor{r, g, b, SELECTION_ALPHA});
                update_selections();
            }

            bool app_editor::visible() const
            {
                INVARIANT(_code_tab);
                INVARIANT(_data_tab);
                return !_code_tab->visibleRegion().isEmpty() || !_data_tab->visibleRegion().isEmpty();
            }

            void app_editor::update_mic_widget()
            {
                INVARIANT(_mic);
                if(_resource.mic)
                {
                    make_green(*_mic);
                    _mic->setToolTip(tr("disable microphone"));
                }
                else
                {
                    make_red(*_mic);
                    _mic->setToolTip(tr("enable microphone"));
                }
            }

            void app_editor::toggle_mic()
            {
                INVARIANT(_front);

                _resource.mic = !_resource.mic;
                update_mic_widget();

                if(_resource.mic) _front->mic_enable();
                else _front->mic_disable();
            }


            lua_highlighter::lua_highlighter(QTextDocument* parent) :
                QSyntaxHighlighter{parent}
            {
                //create api keyword regex
                std::string api_keywords = "\\b(";
                int i = 0;
                for(const auto& p : API_KEYWORDS)
                {
                    if(i != 0) api_keywords.append("|");
                    api_keywords.append(p.first);
                    i++;
                }
                api_keywords.append(")\\b");

                //keywords
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkBlue);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_KEYWORDS.c_str()};
                    _rules.emplace_back(r);
                }

                //api
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkMagenta);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{api_keywords.c_str()};
                    _rules.emplace_back(r);
                }

                //numbers
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkRed);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_NUMBERS.c_str()};
                    _rules.emplace_back(r);
                }

                //operators
                {
                    highlight_rule r;
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_OPERATORS.c_str()};
                    _rules.emplace_back(r);
                }

                //quote
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkGreen);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_QUOTE.c_str()};
                    _rules.emplace_back(r);
                }

                //comment
                {
                    highlight_rule r;
                    r.format.setForeground(Qt::darkGray);
                    r.format.setFontWeight(QFont::Bold);
                    r.regex = QRegExp{LUA_COMMENT.c_str()};
                    _rules.emplace_back(r);
                }
            }

            void lua_highlighter::highlightBlock(const QString& t)
            {
                for(auto& r : _rules)
                {
                    int i = t.indexOf(r.regex);
                    while (i >= 0)  
                    {
                        auto size = r.regex.matchedLength();
                        setFormat(i, size, r.format);
                        i = t.indexOf(r.regex, i + size);
                    }
                }
            }

            data_item::data_item(util::disk_store& d, const std::string& key) :
                _key(key), _d(d)
            {
                auto l = new QHBoxLayout{this};
                setLayout(l);

                std::stringstream ks;
                ks << "<a href='" << key <<"'>" << key << "</a>";
                _key_label = new QLabel{ks.str().c_str()};

                auto v = d.get(key);

                std::stringstream vs;
                if(v.is_int()) vs << v.as_int();
                else if(v.is_size()) vs << v.as_size();
                else if(v.is_double()) vs << v.as_double();
                else if(v.is_dict()) vs << "<complext data>";
                else if(v.is_array()) vs << "<array>";
                else if(v.is_bytes()) 
                {
                    auto b = v.as_bytes();
                    if(b.size() > 100) vs << "<file>";
                    else vs << u::to_str(b);
                }

                _value_label = new QLabel{vs.str().c_str()};

                _rm = new QPushButton;
                make_x(*_rm);
                connect(_rm, SIGNAL(clicked()), this, SLOT(remove()));
                connect(_key_label, SIGNAL(linkActivated(QString)), this, SLOT(key_clicked()));

                l->addWidget(_key_label);
                l->addWidget(_value_label);
                l->addWidget(_rm);

                INVARIANT(_key_label);
                INVARIANT(_value_label);
                INVARIANT(_rm);
            }

            void data_item::remove()
            {
                INVARIANT(_key_label);
                INVARIANT(_value_label);
                INVARIANT(_rm);
                _d.remove(_key);
                setEnabled(false);

                emit data_updated();
            }
            void data_item::key_clicked()
            {
                QString k{_key.c_str()};
                emit key_was_clicked(k);
            }

        }
    }
}

