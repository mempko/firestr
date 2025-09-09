/*
 * Copyright (C) 2025  Maxim Noah Khailo
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
 */

#include "gui/app/qml_app_editor.hpp"
#include "gui/api/service.hpp"
#include "gui/util.hpp"
#include "util/uuid.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QVariantMap>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>

#include <sstream>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace app
        {
            const std::string QML_APP_EDITOR = "qml_app_editor";

            namespace
            {
                const std::string CODE_MESSAGE = "code";
                const std::string CURSOR_MESSAGE = "cursor";
                const std::string DATA_MESSAGE = "data";
                const std::string INIT_MESSAGE = "init";
            }

            // Minimal QML frontend implementation for Lua API
            class qml_app_editor::qml_frontend : public api::frontend
            {
                public:
                    qml_frontend(qml_app_editor* editor) : _editor(editor) {}

                    // Widget management
                    virtual void place(api::ref_id id, int r, int c) override {}
                    virtual void place_across(api::ref_id id, int r, int c, int row_span, int col_span) override {}
                    virtual void widget_enable(api::ref_id, bool) override {}
                    virtual void widget_visible(api::ref_id, bool) override {}
                    virtual void widget_set_style(api::ref_id, const std::string&) override {}

                    // Basic widgets
                    virtual void add_button(api::ref_id, const std::string&) override {}
                    virtual std::string button_get_text(api::ref_id) override { return ""; }
                    virtual void button_set_text(api::ref_id, const std::string&) override {}
                    virtual void button_set_image(api::ref_id, api::ref_id) override {}
                    
                    virtual void add_label(api::ref_id, const std::string&) override {}
                    virtual std::string label_get_text(api::ref_id) override { return ""; }
                    virtual void label_set_text(api::ref_id, const std::string&) override {}
                    
                    virtual void add_edit(api::ref_id, const std::string&) override {}
                    virtual std::string edit_get_text(api::ref_id) override { return ""; }
                    virtual void edit_set_text(api::ref_id, const std::string&) override {}
                    
                    virtual void add_text_edit(api::ref_id, const std::string&) override {}
                    virtual std::string text_edit_get_text(api::ref_id) override { return ""; }
                    virtual void text_edit_set_text(api::ref_id, const std::string&) override {}
                    
                    virtual void add_list(api::ref_id) override {}
                    virtual void add_dropdown(api::ref_id) override {}
                    virtual void add_pen(api::ref_id, const std::string&, int) override {}
                    virtual void add_draw(api::ref_id, int, int) override {}
                    virtual void add_timer(api::ref_id, int) override {}
                    virtual bool add_image(api::ref_id, const util::bytes&) override { return false; }
                    virtual void add_mic(api::ref_id, const std::string&) override {}
                    virtual void add_speaker(api::ref_id, const std::string&) override {}

                    // Grid operations
                    virtual void add_grid(api::ref_id) override {}
                    virtual void grid_place(api::ref_id, api::ref_id, int, int) override {}
                    virtual void grid_place_across(api::ref_id, api::ref_id, int, int, int, int) override {}
                    
                    // Widget visibility
                    virtual bool is_widget_visible(api::ref_id) override { return true; }
                    virtual bool is_widget_enabled(api::ref_id) override { return true; }

                    // List operations
                    virtual void list_add(api::ref_id, api::ref_id) override {}
                    virtual void list_remove(api::ref_id, api::ref_id) override {}
                    virtual size_t list_size(api::ref_id) override { return 0; }
                    virtual void list_clear(api::ref_id) override {}

                    // Dropdown operations
                    virtual size_t dropdown_size(api::ref_id) override { return 0; }
                    virtual void dropdown_add_item(api::ref_id, const std::string&) override {}
                    virtual std::string dropdown_get_item(api::ref_id, int) override { return ""; }
                    virtual int dropdown_get_selected(api::ref_id) override { return -1; }
                    virtual void dropdown_select(api::ref_id, int) override {}
                    virtual void dropdown_clear(api::ref_id) override {}

                    // Drawing operations
                    virtual void draw_line(api::ref_id, api::ref_id, api::ref_id, double, double, double, double) override {}
                    virtual void draw_circle(api::ref_id, api::ref_id, api::ref_id, double, double, double) override {}
                    virtual void draw_image(api::ref_id, api::ref_id, api::ref_id, double, double, double, double) override {}
                    virtual void draw_clear(api::ref_id) override {}
                    virtual void draw_line_set(api::ref_id, api::ref_id, double, double, double, double) override {}
                    virtual void draw_line_set_pen(api::ref_id, api::ref_id, api::ref_id) override {}
                    virtual void draw_circle_set(api::ref_id, api::ref_id, double, double, double) override {}
                    virtual void draw_circle_set_pen(api::ref_id, api::ref_id, api::ref_id) override {}
                    virtual void draw_image_set(api::ref_id, api::ref_id, double, double, double, double) override {}
                    virtual void pen_set_width(api::ref_id, int) override {}

                    // Timer operations
                    virtual bool timer_running(api::ref_id) override { return false; }
                    virtual void timer_stop(api::ref_id) override {}
                    virtual void timer_start(api::ref_id) override {}
                    virtual void timer_set_interval(api::ref_id, int) override {}

                    // Image operations
                    virtual int image_width(api::ref_id) override { return 0; }
                    virtual int image_height(api::ref_id) override { return 0; }

                    // Audio operations
                    virtual void mic_start(api::ref_id) override {}
                    virtual void mic_stop(api::ref_id) override {}
                    virtual void mic_disable() override {}
                    virtual void mic_enable() override {}
                    virtual bool mic_enabled() const override { return false; }
                    virtual void speaker_mute(api::ref_id) override {}
                    virtual void speaker_unmute(api::ref_id) override {}
                    virtual void speaker_play(api::ref_id, const util::bytes&) override {}

                    // File operations
                    virtual api::file_data open_file() override { return api::file_data{"", "", false}; }
                    virtual api::bin_file_data open_bin_file() override { return api::bin_file_data{"", util::bytes{}, false}; }
                    virtual bool save_file(const std::string&, const std::string&) override { return false; }
                    virtual bool save_bin_file(const std::string&, const util::bytes&) override { return false; }

                    // Debug/output
                    virtual void print(const std::string& text) override 
                    {
                        if(_editor) _editor->append_output(QString::fromStdString(text) + "\n");
                    }

                    // GUI operations
                    virtual void height(int) override {}
                    virtual void width(int) override {}
                    virtual bool visible() override { return true; }
                    virtual void grow() override {}
                    virtual void alert() override {}
                    virtual void report_error(const std::string& e) override 
                    {
                        if(_editor) _editor->append_output(QString("Error: %1\n").arg(QString::fromStdString(e)));
                    }
                    virtual void adjust_size() override {}
                    virtual void reset() override {}

                private:
                    qml_app_editor* _editor;
            };

            qml_app_editor::qml_app_editor(
                    app_service_ptr app_service,
                    app_reaper_ptr app_reaper,
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    app_ptr a,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{u::uuid()},
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_service},
                _conversation{conversation},
                _app{a},
                _can_save{false},
                _is_running{false},
                _cursor_position{0},
                _code_changed{false},
                _updating{false}
            {
                REQUIRE(app_service);
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                init();
            }

            qml_app_editor::qml_app_editor(
                    const std::string& from_id,
                    const std::string& id,
                    app_service_ptr app_service,
                    app_reaper_ptr app_reaper,
                    s::conversation_service_ptr conversation_service,
                    s::conversation_ptr conversation,
                    app_ptr a,
                    QObject* parent) :
                qml_generic_app{parent},
                _id{id},
                _from_id{from_id},
                _app_service{app_service},
                _app_reaper{app_reaper},
                _conversation_service{conversation_service},
                _conversation{conversation},
                _app{a},
                _can_save{false},
                _is_running{false},
                _cursor_position{0},
                _code_changed{false},
                _updating{false}
            {
                REQUIRE(app_service);
                REQUIRE(conversation_service);
                REQUIRE(conversation);
                REQUIRE_FALSE(id.empty());
                init();
            }

            qml_app_editor::~qml_app_editor()
            {
                stopScript();
            }

            void qml_app_editor::init()
            {
                INVARIANT(_conversation_service);
                INVARIANT(_conversation);
                INVARIANT(_app_service);

                _mail = std::make_shared<m::mailbox>(_id);
                _sender = std::make_shared<ms::sender>(_conversation_service->user_service(), _mail);

                // Initialize with default Lua code
                _code = "-- Fire★ App Editor\n-- Write your Lua code here\n\nfunction main()\n    print('Hello from Fire★!')\nend\n\nmain()";
                _status = "Ready";
                _app_name = "New App";

                // Create frontend for Lua API
                _frontend = std::make_unique<qml_frontend>(this);

                // Create or use existing app
                if(!_app)
                {
                    _app = _app_service->create_new_app();
                    _app->name("New App");
                    _app->code(_code.toStdString());
                }
                
                // Initialize Lua API with proper context
                _api = std::make_shared<lua::lua_api>(
                    _app,
                    _sender,
                    _conversation,
                    _conversation_service,
                    _frontend.get()
                );

                // If we have an existing app, load it
                if(_app && !_app->name().empty() && _app->name() != "temp")
                {
                    _app_name = QString::fromStdString(_app->name());
                    _code = QString::fromStdString(_app->code());
                    init_data();
                    emit appNameChanged();
                    emit codeChanged();
                    emit dataItemsChanged();
                }

                init_handlers();

                INVARIANT(_mail);
                INVARIANT(_sender);
                INVARIANT(_api);
                INVARIANT(_frontend);
            }

            void qml_app_editor::init_handlers()
            {
                // TODO: Connect to mailbox for receiving messages
            }

            void qml_app_editor::init_data()
            {
                if(!_app) return;

                _data_items.clear();
                
                // Load app data into QVariantList
                for(const auto& kv : _app->data())
                {
                    QVariantMap item;
                    item["key"] = QString::fromStdString(kv.first);
                    item["value"] = QByteArray::fromStdString(kv.second);
                    _data_items.append(item);
                }
            }

            void qml_app_editor::start()
            {
                INVARIANT(_mail);
                
                update_status("Ready");
                append_output("App Editor started\n");
            }

            void qml_app_editor::contact_quit(const std::string& id)
            {
                INVARIANT(_conversation);
                
                // Remove collaborator cursor
                auto it = _collaborators.find(id);
                if(it != _collaborators.end())
                {
                    _collaborators.erase(it);
                    emit collaboratorsChanged();
                }
            }

            QString qml_app_editor::code() const
            {
                return _code;
            }

            void qml_app_editor::setCode(const QString& code)
            {
                if(_code != code)
                {
                    _code = code;
                    _code_changed = true;
                    _can_save = true;
                    emit codeChanged();
                    emit canSaveChanged();
                    
                    // Send code to collaborators
                    if(!_updating)
                    {
                        sendScript();
                    }
                }
            }

            QString qml_app_editor::output() const
            {
                return _output;
            }

            QString qml_app_editor::status() const
            {
                return _status;
            }

            QString qml_app_editor::appName() const
            {
                return _app_name;
            }

            void qml_app_editor::setAppName(const QString& name)
            {
                if(_app_name != name)
                {
                    _app_name = name;
                    _can_save = true;
                    emit appNameChanged();
                    emit canSaveChanged();
                }
            }

            QVariantList qml_app_editor::dataItems() const
            {
                return _data_items;
            }

            bool qml_app_editor::canSave() const
            {
                return _can_save && !_app_name.isEmpty();
            }

            bool qml_app_editor::isRunning() const
            {
                return _is_running;
            }

            int qml_app_editor::cursorPosition() const
            {
                return _cursor_position;
            }

            void qml_app_editor::setCursorPosition(int pos)
            {
                if(_cursor_position != pos)
                {
                    _cursor_position = pos;
                    emit cursorPositionChanged();
                    
                    // Send cursor position to collaborators
                    send_cursor_position();
                }
            }

            QVariantList qml_app_editor::collaborators() const
            {
                QVariantList result;
                for(const auto& kv : _collaborators)
                {
                    QVariantMap collaborator;
                    collaborator["id"] = QString::fromStdString(kv.first);
                    collaborator["name"] = QString::fromStdString(kv.second.name);
                    collaborator["cursorPos"] = kv.second.cursor_pos;
                    collaborator["color"] = kv.second.color;
                    result.append(collaborator);
                }
                return result;
            }

            const std::string& qml_app_editor::id() const
            {
                ENSURE_FALSE(_id.empty());
                return _id;
            }

            const std::string& qml_app_editor::type() const
            {
                ENSURE_FALSE(QML_APP_EDITOR.empty());
                return QML_APP_EDITOR;
            }

            m::mailbox_ptr qml_app_editor::mail()
            {
                ENSURE(_mail);
                return _mail;
            }

            void qml_app_editor::runScript()
            {
                INVARIANT(_api);
                INVARIANT(_app);
                
                _is_running = true;
                emit isRunningChanged();
                emit scriptStarted();
                
                update_status("Running...");
                clearOutput();
                
                try
                {
                    // Update app code
                    _app->code(_code.toStdString());
                    
                    // Reset and execute the Lua code
                    _api->reset();
                    _api->run(_app->code());
                    
                    update_status("Completed");
                    append_output("\n--- Script completed ---\n");
                }
                catch(const std::exception& e)
                {
                    update_status("Error");
                    append_output(QString("\nError: %1\n").arg(e.what()));
                    emit errorOccurred(e.what());
                }
                
                _is_running = false;
                emit isRunningChanged();
                emit scriptStopped();
            }

            void qml_app_editor::stopScript()
            {
                if(!_is_running) return;
                
                // TODO: Implement script interruption
                _is_running = false;
                emit isRunningChanged();
                emit scriptStopped();
                
                update_status("Stopped");
                append_output("\n--- Script stopped ---\n");
            }

            void qml_app_editor::saveApp()
            {
                if(!canSave()) return;
                
                INVARIANT(_app_service);
                
                // Create or update app
                if(!_app)
                {
                    _app = _app_service->create_new_app();
                    _app->name(_app_name.toStdString());
                    _app->code(_code.toStdString());
                }
                else
                {
                    _app->name(_app_name.toStdString());
                    _app->code(_code.toStdString());
                }
                
                // Save data items
                for(const auto& item : _data_items)
                {
                    auto map = item.toMap();
                    auto key = map["key"].toString().toStdString();
                    auto value = map["value"].toByteArray().toStdString();
                    _app->data().set(key, u::to_bytes(value));
                }
                
                // Save to app service
                _app_service->save_app(*_app);
                
                _can_save = false;
                emit canSaveChanged();
                
                update_status("Saved");
                append_output(QString("App '%1' saved.\n").arg(_app_name));
            }

            void qml_app_editor::exportApp()
            {
                // TODO: Implement app export
                append_output("Export not yet implemented.\n");
            }

            void qml_app_editor::clearOutput()
            {
                _output.clear();
                emit outputChanged();
            }

            void qml_app_editor::sendScript()
            {
                INVARIANT(_sender);
                INVARIANT(_conversation);
                
                m::message msg;
                msg.meta.type = CODE_MESSAGE;
                msg.meta.extra["from"] = _conversation_service->user_service()->user().info().name();
                msg.data = u::to_bytes(_code.toStdString());
                
                send_all(msg);
            }

            void qml_app_editor::addDataItem()
            {
                QVariantMap item;
                item["key"] = QString("data_%1").arg(_data_items.size());
                item["value"] = QByteArray();
                _data_items.append(item);
                
                _can_save = true;
                emit dataItemsChanged();
                emit canSaveChanged();
            }

            void qml_app_editor::removeDataItem(const QString& key)
            {
                for(int i = 0; i < _data_items.size(); ++i)
                {
                    auto map = _data_items[i].toMap();
                    if(map["key"].toString() == key)
                    {
                        _data_items.removeAt(i);
                        _can_save = true;
                        emit dataItemsChanged();
                        emit canSaveChanged();
                        break;
                    }
                }
            }

            void qml_app_editor::updateDataItem(const QString& key, const QVariant& value)
            {
                for(int i = 0; i < _data_items.size(); ++i)
                {
                    auto map = _data_items[i].toMap();
                    if(map["key"].toString() == key)
                    {
                        map["value"] = value;
                        _data_items[i] = map;
                        _can_save = true;
                        emit dataItemsChanged();
                        emit canSaveChanged();
                        break;
                    }
                }
            }

            void qml_app_editor::loadDataFromFile(const QString& key)
            {
                // TODO: Implement file loading
                append_output("Load from file not yet implemented.\n");
            }

            void qml_app_editor::send_all(const m::message& msg)
            {
                INVARIANT(_conversation);
                INVARIANT(_sender);
                
                for(const auto& c : _conversation->contacts().list())
                {
                    if(c) _sender->send(c->id(), msg);
                }
            }

            void qml_app_editor::send_cursor_position()
            {
                INVARIANT(_sender);
                INVARIANT(_conversation);
                
                m::message msg;
                msg.meta.type = CURSOR_MESSAGE;
                msg.meta.extra["from"] = _conversation_service->user_service()->user().info().name();
                msg.meta.extra["pos"] = _cursor_position;
                
                send_all(msg);
            }

            void qml_app_editor::update_status(const QString& status)
            {
                if(_status != status)
                {
                    _status = status;
                    emit statusChanged();
                }
            }

            void qml_app_editor::append_output(const QString& text)
            {
                _output += text;
                emit outputChanged();
                emit outputAppended(text);
            }

            void qml_app_editor::handle_remote_code(const m::message& msg)
            {
                auto code = QString::fromStdString(u::to_str(msg.data));
                auto from = msg.meta.extra["from"].as_string();
                
                _updating = true;
                setCode(code);
                _updating = false;
                
                append_output(QString("Code updated by %1\n").arg(QString::fromStdString(from)));
            }

            void qml_app_editor::handle_remote_cursor(const m::message& msg)
            {
                auto from = msg.meta.extra["from"].as_string();
                auto pos = msg.meta.extra["pos"].as_int();
                
                update_collaborator_cursor(from, pos);
            }

            void qml_app_editor::handle_remote_data(const m::message& msg)
            {
                // TODO: Handle remote data updates
            }

            void qml_app_editor::update_collaborator_cursor(const std::string& id, int pos)
            {
                // Get or create collaborator info
                auto& info = _collaborators[id];
                if(info.name.empty())
                {
                    auto contact = _conversation->contacts().by_id(id);
                    if(contact)
                    {
                        info.name = contact->name();
                    }
                    else
                    {
                        info.name = id;
                    }
                    
                    // Assign a color based on ID hash
                    auto hash = std::hash<std::string>{}(id);
                    info.color = QColor::fromHsv((hash % 360), 200, 200);
                }
                
                info.cursor_pos = pos;
                emit collaboratorsChanged();
            }

            void qml_app_editor::update_app_code()
            {
                if(!_app) return;
                
                _app->code(_code.toStdString());
                _app->name(_app_name.toStdString());
            }
        }
    }
}