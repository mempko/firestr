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

#include "gui/app/qml_lua_frontend.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QQmlContext>
#include <QQuickWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QThread>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            namespace
            {
                const char* GRID_QML = R"(
                    import QtQuick
                    import QtQuick.Layouts
                    
                    GridLayout {
                        id: grid
                        columns: 3
                        rowSpacing: 5
                        columnSpacing: 5
                    }
                )";
                
                const char* BUTTON_QML = R"(
                    import QtQuick
                    import QtQuick.Controls
                    
                    Button {
                        id: button
                        property int widgetId: 0
                        
                        background: Rectangle {
                            color: parent.pressed ? "#388E3C" : (parent.hovered ? "#66BB6A" : "#4CAF50")
                            radius: 3
                        }
                        
                        contentItem: Text {
                            text: button.text
                            color: "white"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                )";
                
                const char* LABEL_QML = R"(
                    import QtQuick
                    
                    Text {
                        id: label
                        property int widgetId: 0
                        color: "#e0e0e0"
                        font.pixelSize: 12
                    }
                )";
                
                const char* EDIT_QML = R"(
                    import QtQuick
                    import QtQuick.Controls
                    
                    TextField {
                        id: edit
                        property int widgetId: 0
                        
                        background: Rectangle {
                            color: "#2b2b2b"
                            border.color: edit.focus ? "#4CAF50" : "#3c3c3c"
                            border.width: 1
                            radius: 3
                        }
                        
                        color: "#e0e0e0"
                        font.pixelSize: 12
                        selectByMouse: true
                    }
                )";
                
                const char* TEXT_EDIT_QML = R"(
                    import QtQuick
                    import QtQuick.Controls
                    
                    ScrollView {
                        id: scrollView
                        property int widgetId: 0
                        property alias text: textArea.text
                        
                        TextArea {
                            id: textArea
                            color: "#e0e0e0"
                            font.pixelSize: 12
                            font.family: "Consolas, Monaco, monospace"
                            selectByMouse: true
                            wrapMode: TextArea.Wrap
                            
                            background: Rectangle {
                                color: "#2b2b2b"
                                border.color: textArea.focus ? "#4CAF50" : "#3c3c3c"
                                border.width: 1
                                radius: 3
                            }
                        }
                    }
                )";
                
                const char* LIST_QML = R"(
                    import QtQuick
                    import QtQuick.Controls
                    
                    Rectangle {
                        id: listContainer
                        property int widgetId: 0
                        color: "#2b2b2b"
                        border.color: "#3c3c3c"
                        border.width: 1
                        radius: 3
                        
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 5
                            
                            Column {
                                id: listContent
                                spacing: 5
                            }
                        }
                    }
                )";
                
                const char* DROPDOWN_QML = R"(
                    import QtQuick
                    import QtQuick.Controls
                    
                    ComboBox {
                        id: dropdown
                        property int widgetId: 0
                        
                        background: Rectangle {
                            color: "#2b2b2b"
                            border.color: dropdown.focus ? "#4CAF50" : "#3c3c3c"
                            border.width: 1
                            radius: 3
                        }
                        
                        contentItem: Text {
                            text: dropdown.displayText
                            color: "#e0e0e0"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 10
                        }
                    }
                )";
                
                const char* CANVAS_QML = R"(
                    import QtQuick
                    
                    Canvas {
                        id: canvas
                        property int widgetId: 0
                        
                        property var drawCommands: []
                        
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            
                            for(var i = 0; i < drawCommands.length; i++) {
                                var cmd = drawCommands[i]
                                executeDrawCommand(ctx, cmd)
                            }
                        }
                        
                        function executeDrawCommand(ctx, cmd) {
                            if(cmd.type === "line") {
                                ctx.strokeStyle = cmd.color || "#ffffff"
                                ctx.lineWidth = cmd.width || 1
                                ctx.beginPath()
                                ctx.moveTo(cmd.x1, cmd.y1)
                                ctx.lineTo(cmd.x2, cmd.y2)
                                ctx.stroke()
                            } else if(cmd.type === "circle") {
                                ctx.strokeStyle = cmd.color || "#ffffff"
                                ctx.lineWidth = cmd.width || 1
                                ctx.beginPath()
                                ctx.arc(cmd.x, cmd.y, cmd.radius, 0, 2 * Math.PI)
                                ctx.stroke()
                            }
                        }
                        
                        function addDrawCommand(cmd) {
                            drawCommands.push(cmd)
                            requestPaint()
                        }
                        
                        function clearDrawCommands() {
                            drawCommands = []
                            requestPaint()
                        }
                    }
                )";
                
                const char* TIMER_QML = R"(
                    import QtQuick
                    
                    Timer {
                        id: timer
                        property int widgetId: 0
                        repeat: true
                    }
                )";
                
                const char* IMAGE_QML = R"(
                    import QtQuick
                    
                    Image {
                        id: image
                        property int widgetId: 0
                        fillMode: Image.PreserveAspectFit
                    }
                )";
            }
            
            qml_lua_frontend::qml_lua_frontend(QQmlEngine* engine, QObject* parent) :
                QObject{parent},
                _engine{engine},
                _root_item{nullptr},
                _grid_component{nullptr},
                _button_component{nullptr},
                _label_component{nullptr},
                _edit_component{nullptr},
                _text_edit_component{nullptr},
                _list_component{nullptr},
                _dropdown_component{nullptr},
                _canvas_component{nullptr},
                _timer_component{nullptr},
                _image_component{nullptr},
                _mic_enabled{true},
                _initialized{false}
            {
                REQUIRE(engine);
                
                LOG << "qml_lua_frontend constructor on thread: " << QThread::currentThread() 
                    << " (main thread: " << qApp->thread() << ")" << std::endl;
                
                // Connect initialization signal first
                connect(this, &qml_lua_frontend::requestInitialize,
                        this, &qml_lua_frontend::handleInitialize,
                        Qt::QueuedConnection);
                
                // Connect signals to slots for thread-safe widget creation
                connect(this, &qml_lua_frontend::requestAddButton,
                        this, &qml_lua_frontend::handleAddButton,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestAddLabel,
                        this, &qml_lua_frontend::handleAddLabel,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestAddEdit,
                        this, &qml_lua_frontend::handleAddEdit,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestAddTextEdit,
                        this, &qml_lua_frontend::handleAddTextEdit,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestAddList,
                        this, &qml_lua_frontend::handleAddList,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestAddGrid,
                        this, &qml_lua_frontend::handleAddGrid,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestPlace,
                        this, &qml_lua_frontend::handlePlace,
                        Qt::QueuedConnection);
                connect(this, &qml_lua_frontend::requestPlaceAcross,
                        this, &qml_lua_frontend::handlePlaceAcross,
                        Qt::QueuedConnection);
            }
            
            void qml_lua_frontend::initialize()
            {
                LOG << "Requesting initialization" << std::endl;
                emit requestInitialize();
            }
            
            void qml_lua_frontend::handleInitialize()
            {
                if(_initialized) return;
                
                LOG << "Initializing QML components on thread: " << QThread::currentThread() 
                    << " (main thread: " << qApp->thread() << ")" << std::endl;
                
                // Create root item
                QQmlComponent rootComponent{_engine};
                rootComponent.setData(R"(
                    import QtQuick
                    import QtQuick.Layouts
                    
                    Rectangle {
                        color: "#1e1e1e"
                        
                        GridLayout {
                            id: mainGrid
                            objectName: "mainGrid"
                            anchors.fill: parent
                            anchors.margins: 10
                            columns: 3
                            rowSpacing: 5
                            columnSpacing: 5
                        }
                    }
                )", QUrl{});
                
                _root_item = qobject_cast<QQuickItem*>(rootComponent.create());
                if(_root_item)
                {
                    _root_item->setParentItem(nullptr);
                    LOG << "QML Lua frontend root item created" << std::endl;
                    emit rootItemChanged();
                }
                else
                {
                    LOG << "Failed to create QML Lua frontend root item" << std::endl;
                }
                
                // Create component templates
                _grid_component = new QQmlComponent{_engine, this};
                _grid_component->setData(GRID_QML, QUrl{});
                
                _button_component = new QQmlComponent{_engine, this};
                _button_component->setData(BUTTON_QML, QUrl{});
                
                _label_component = new QQmlComponent{_engine, this};
                _label_component->setData(LABEL_QML, QUrl{});
                
                _edit_component = new QQmlComponent{_engine, this};
                _edit_component->setData(EDIT_QML, QUrl{});
                
                _text_edit_component = new QQmlComponent{_engine, this};
                _text_edit_component->setData(TEXT_EDIT_QML, QUrl{});
                
                _list_component = new QQmlComponent{_engine, this};
                _list_component->setData(LIST_QML, QUrl{});
                
                _dropdown_component = new QQmlComponent{_engine, this};
                _dropdown_component->setData(DROPDOWN_QML, QUrl{});
                
                _canvas_component = new QQmlComponent{_engine, this};
                _canvas_component->setData(CANVAS_QML, QUrl{});
                
                _timer_component = new QQmlComponent{_engine, this};
                _timer_component->setData(TIMER_QML, QUrl{});
                
                _image_component = new QQmlComponent{_engine, this};
                _image_component->setData(IMAGE_QML, QUrl{});
                
                _initialized = true;
                LOG << "QML components initialized successfully" << std::endl;
            }
            
            qml_lua_frontend::~qml_lua_frontend()
            {
                // Clean up widgets
                for(auto& kv : _widgets)
                {
                    if(kv.second) kv.second->deleteLater();
                }
                _widgets.clear();
                
                if(_root_item) _root_item->deleteLater();
            }
            
            QQuickItem* qml_lua_frontend::createWidget(const QString& type, api::ref_id id)
            {
                if(!_initialized)
                {
                    LOG << "ERROR: Attempting to create widget before initialization!" << std::endl;
                    return nullptr;
                }
                
                QQmlComponent* component = nullptr;
                
                if(type == "grid") component = _grid_component;
                else if(type == "button") component = _button_component;
                else if(type == "label") component = _label_component;
                else if(type == "edit") component = _edit_component;
                else if(type == "text_edit") component = _text_edit_component;
                else if(type == "list") component = _list_component;
                else if(type == "dropdown") component = _dropdown_component;
                else if(type == "canvas") component = _canvas_component;
                else if(type == "timer") component = _timer_component;
                else if(type == "image") component = _image_component;
                
                if(!component) 
                {
                    LOG << "ERROR: No component for type: " << type.toStdString() << std::endl;
                    return nullptr;
                }
                
                auto* item = qobject_cast<QQuickItem*>(component->create());
                if(item)
                {
                    item->setProperty("widgetId", static_cast<int>(id));
                    _widgets[id] = item;
                    
                    // Connect signals based on type
                    if(type == "button")
                    {
                        // For now, we'll handle button clicks through QML bindings
                        // TODO: Connect button signals properly
                    }
                    else if(type == "edit")
                    {
                        // TODO: Connect edit signals properly
                    }
                    else if(type == "text_edit")
                    {
                        // TODO: Connect text edit signals properly
                    }
                    else if(type == "dropdown")
                    {
                        // TODO: Connect dropdown signals properly
                    }
                    else if(type == "timer")
                    {
                        // TODO: Connect timer signals properly
                    }
                }
                
                return item;
            }
            
            QQuickItem* qml_lua_frontend::getWidget(api::ref_id id)
            {
                auto it = _widgets.find(id);
                return it != _widgets.end() ? it->second : nullptr;
            }
            
            void qml_lua_frontend::setProperty(QQuickItem* item, const QString& property, const QVariant& value)
            {
                if(item) item->setProperty(property.toUtf8().constData(), value);
            }
            
            QVariant qml_lua_frontend::getProperty(QQuickItem* item, const QString& property)
            {
                return item ? item->property(property.toUtf8().constData()) : QVariant{};
            }
            
            // Widget placement
            void qml_lua_frontend::place(api::ref_id id, int r, int c)
            {
                emit requestPlace(static_cast<quint64>(id), r, c);
            }
            
            void qml_lua_frontend::handlePlace(quint64 id, int r, int c)
            {
                LOG << "Placing widget " << id << " at (" << r << "," << c << ")" << std::endl;
                
                auto* widget = getWidget(static_cast<api::ref_id>(id));
                if(!widget) 
                {
                    LOG << "ERROR: Widget not found for id " << id << std::endl;
                    return;
                }
                
                if(!_root_item)
                {
                    LOG << "ERROR: No root item for placement" << std::endl;
                    return;
                }
                
                auto* mainGrid = _root_item->findChild<QQuickItem*>("mainGrid");
                if(!mainGrid) 
                {
                    LOG << "ERROR: mainGrid not found in root item" << std::endl;
                    LOG << "Root item children:" << std::endl;
                    auto children = _root_item->childItems();
                    for(auto* child : children)
                    {
                        LOG << "  - " << child->objectName().toStdString() << " (" << child->metaObject()->className() << ")" << std::endl;
                    }
                    return;
                }
                
                LOG << "Setting parent and layout properties for widget" << std::endl;
                widget->setParentItem(mainGrid);
                setProperty(widget, QString("Layout.row"), r);
                setProperty(widget, QString("Layout.column"), c);
                LOG << "Widget placed successfully" << std::endl;
            }
            
            void qml_lua_frontend::place_across(api::ref_id id, int r, int c, int row_span, int col_span)
            {
                emit requestPlaceAcross(static_cast<quint64>(id), r, c, row_span, col_span);
            }
            
            void qml_lua_frontend::handlePlaceAcross(quint64 id, int r, int c, int row_span, int col_span)
            {
                auto* widget = getWidget(static_cast<api::ref_id>(id));
                if(!widget || !_root_item) return;
                
                auto* mainGrid = _root_item->findChild<QQuickItem*>("mainGrid");
                if(!mainGrid) return;
                
                widget->setParentItem(mainGrid);
                setProperty(widget, QString("Layout.row"), r);
                setProperty(widget, QString("Layout.column"), c);
                setProperty(widget, QString("Layout.rowSpan"), row_span);
                setProperty(widget, QString("Layout.columnSpan"), col_span);
            }
            
            void qml_lua_frontend::widget_enable(api::ref_id id, bool enable)
            {
                auto* widget = getWidget(id);
                if(widget) widget->setEnabled(enable);
            }
            
            bool qml_lua_frontend::is_widget_enabled(api::ref_id id)
            {
                auto* widget = getWidget(id);
                return widget ? widget->isEnabled() : false;
            }
            
            void qml_lua_frontend::widget_visible(api::ref_id id, bool visible)
            {
                auto* widget = getWidget(id);
                if(widget) widget->setVisible(visible);
            }
            
            bool qml_lua_frontend::is_widget_visible(api::ref_id id)
            {
                auto* widget = getWidget(id);
                return widget ? widget->isVisible() : false;
            }
            
            void qml_lua_frontend::widget_set_style(api::ref_id id, const std::string& style)
            {
                // QML handles styles differently - this might need custom implementation
                // For now, we'll skip style application
            }
            
            // Grid
            void qml_lua_frontend::add_grid(api::ref_id id)
            {
                emit requestAddGrid(static_cast<quint64>(id));
            }
            
            void qml_lua_frontend::handleAddGrid(quint64 id)
            {
                createWidget("grid", static_cast<api::ref_id>(id));
            }
            
            void qml_lua_frontend::grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c)
            {
                auto* grid = getWidget(grid_id);
                auto* widget = getWidget(widget_id);
                if(!grid || !widget) return;
                
                widget->setParentItem(grid);
                setProperty(widget, QString("Layout.row"), r);
                setProperty(widget, QString("Layout.column"), c);
            }
            
            void qml_lua_frontend::grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span)
            {
                auto* grid = getWidget(grid_id);
                auto* widget = getWidget(widget_id);
                if(!grid || !widget) return;
                
                widget->setParentItem(grid);
                setProperty(widget, QString("Layout.row"), r);
                setProperty(widget, QString("Layout.column"), c);
                setProperty(widget, QString("Layout.rowSpan"), row_span);
                setProperty(widget, QString("Layout.columnSpan"), col_span);
            }
            
            // Button
            void qml_lua_frontend::add_button(api::ref_id id, const std::string& text)
            {
                LOG << "Requesting button creation - id: " << id << ", text: " << text << std::endl;
                emit requestAddButton(static_cast<quint64>(id), QString::fromStdString(text));
            }
            
            void qml_lua_frontend::handleAddButton(quint64 id, const QString& text)
            {
                LOG << "Creating button on thread: " << QThread::currentThread() 
                    << " (main thread: " << qApp->thread() << ")" << std::endl;
                LOG << "Creating button - id: " << id << ", text: " << text.toStdString() << std::endl;
                
                auto* button = createWidget("button", static_cast<api::ref_id>(id));
                if(button) 
                {
                    setProperty(button, "text", text);
                    LOG << "Button created successfully" << std::endl;
                }
                else
                {
                    LOG << "Failed to create button widget" << std::endl;
                }
            }
            
            std::string qml_lua_frontend::button_get_text(api::ref_id id)
            {
                auto* button = getWidget(id);
                if(!button) return "";
                return getProperty(button, "text").toString().toStdString();
            }
            
            void qml_lua_frontend::button_set_text(api::ref_id id, const std::string& text)
            {
                auto* button = getWidget(id);
                if(button) setProperty(button, "text", QString::fromStdString(text));
            }
            
            void qml_lua_frontend::button_set_image(api::ref_id button_id, api::ref_id image_id)
            {
                // TODO: Implement button image support
            }
            
            // Label
            void qml_lua_frontend::add_label(api::ref_id id, const std::string& text)
            {
                LOG << "Requesting label creation - id: " << id << ", text: " << text << std::endl;
                emit requestAddLabel(static_cast<quint64>(id), QString::fromStdString(text));
            }
            
            void qml_lua_frontend::handleAddLabel(quint64 id, const QString& text)
            {
                LOG << "Creating label on main thread - id: " << id << std::endl;
                auto* label = createWidget("label", static_cast<api::ref_id>(id));
                if(label) setProperty(label, "text", text);
            }
            
            std::string qml_lua_frontend::label_get_text(api::ref_id id)
            {
                auto* label = getWidget(id);
                if(!label) return "";
                return getProperty(label, "text").toString().toStdString();
            }
            
            void qml_lua_frontend::label_set_text(api::ref_id id, const std::string& text)
            {
                auto* label = getWidget(id);
                if(label) setProperty(label, "text", QString::fromStdString(text));
            }
            
            // Edit
            void qml_lua_frontend::add_edit(api::ref_id id, const std::string& text)
            {
                LOG << "Requesting edit creation - id: " << id << ", text: " << text << std::endl;
                emit requestAddEdit(static_cast<quint64>(id), QString::fromStdString(text));
            }
            
            void qml_lua_frontend::handleAddEdit(quint64 id, const QString& text)
            {
                LOG << "Creating edit on main thread - id: " << id << std::endl;
                auto* edit = createWidget("edit", static_cast<api::ref_id>(id));
                if(edit) 
                {
                    setProperty(edit, "text", text);
                    LOG << "Edit created successfully" << std::endl;
                }
                else
                {
                    LOG << "Failed to create edit widget" << std::endl;
                }
            }
            
            std::string qml_lua_frontend::edit_get_text(api::ref_id id)
            {
                auto* edit = getWidget(id);
                if(!edit) return "";
                return getProperty(edit, "text").toString().toStdString();
            }
            
            void qml_lua_frontend::edit_set_text(api::ref_id id, const std::string& text)
            {
                auto* edit = getWidget(id);
                if(edit) setProperty(edit, "text", QString::fromStdString(text));
            }
            
            // Text Edit
            void qml_lua_frontend::add_text_edit(api::ref_id id, const std::string& text)
            {
                emit requestAddTextEdit(static_cast<quint64>(id), QString::fromStdString(text));
            }
            
            void qml_lua_frontend::handleAddTextEdit(quint64 id, const QString& text)
            {
                auto* textEdit = createWidget("text_edit", static_cast<api::ref_id>(id));
                if(textEdit) setProperty(textEdit, "text", text);
            }
            
            std::string qml_lua_frontend::text_edit_get_text(api::ref_id id)
            {
                auto* textEdit = getWidget(id);
                if(!textEdit) return "";
                return getProperty(textEdit, "text").toString().toStdString();
            }
            
            void qml_lua_frontend::text_edit_set_text(api::ref_id id, const std::string& text)
            {
                auto* textEdit = getWidget(id);
                if(textEdit) setProperty(textEdit, "text", QString::fromStdString(text));
            }
            
            // List
            void qml_lua_frontend::add_list(api::ref_id id)
            {
                LOG << "Requesting list creation - id: " << id << std::endl;
                emit requestAddList(static_cast<quint64>(id));
            }
            
            void qml_lua_frontend::handleAddList(quint64 id)
            {
                LOG << "Creating list on main thread - id: " << id << std::endl;
                auto rid = static_cast<api::ref_id>(id);
                auto* list = createWidget("list", rid);
                if(list)
                {
                    _widget_data[rid] = QVariantMap{};
                    LOG << "List created successfully" << std::endl;
                }
                else
                {
                    LOG << "Failed to create list widget" << std::endl;
                }
            }
            
            void qml_lua_frontend::list_add(api::ref_id list_id, api::ref_id widget_id)
            {
                auto* list = getWidget(list_id);
                auto* widget = getWidget(widget_id);
                if(!list || !widget) return;
                
                auto* content = list->findChild<QQuickItem*>("listContent");
                if(content) widget->setParentItem(content);
            }
            
            void qml_lua_frontend::list_remove(api::ref_id list_id, api::ref_id widget_id)
            {
                auto* widget = getWidget(widget_id);
                if(widget) widget->setParentItem(nullptr);
            }
            
            size_t qml_lua_frontend::list_size(api::ref_id id)
            {
                auto* list = getWidget(id);
                if(!list) return 0;
                
                auto* content = list->findChild<QQuickItem*>("listContent");
                if(!content) return 0;
                
                return content->childItems().size();
            }
            
            void qml_lua_frontend::list_clear(api::ref_id id)
            {
                auto* list = getWidget(id);
                if(!list) return;
                
                auto* content = list->findChild<QQuickItem*>("listContent");
                if(!content) return;
                
                for(auto* child : content->childItems())
                {
                    child->deleteLater();
                }
            }
            
            // Dropdown
            void qml_lua_frontend::add_dropdown(api::ref_id id)
            {
                createWidget("dropdown", id);
                _widget_data[id] = QVariantMap{};
            }
            
            size_t qml_lua_frontend::dropdown_size(api::ref_id id)
            {
                auto* dropdown = getWidget(id);
                if(!dropdown) return 0;
                
                auto model = getProperty(dropdown, "model");
                if(model.canConvert<QStringList>())
                {
                    return model.toStringList().size();
                }
                return 0;
            }
            
            void qml_lua_frontend::dropdown_add_item(api::ref_id id, const std::string& item)
            {
                auto* dropdown = getWidget(id);
                if(!dropdown) return;
                
                auto model = getProperty(dropdown, "model");
                QStringList items;
                if(model.canConvert<QStringList>())
                {
                    items = model.toStringList();
                }
                items.append(QString::fromStdString(item));
                setProperty(dropdown, "model", items);
            }
            
            std::string qml_lua_frontend::dropdown_get_item(api::ref_id id, int index)
            {
                auto* dropdown = getWidget(id);
                if(!dropdown) return "";
                
                auto model = getProperty(dropdown, "model");
                if(model.canConvert<QStringList>())
                {
                    auto items = model.toStringList();
                    if(index >= 0 && index < items.size())
                    {
                        return items[index].toStdString();
                    }
                }
                return "";
            }
            
            int qml_lua_frontend::dropdown_get_selected(api::ref_id id)
            {
                auto* dropdown = getWidget(id);
                if(!dropdown) return -1;
                return getProperty(dropdown, "currentIndex").toInt();
            }
            
            void qml_lua_frontend::dropdown_select(api::ref_id id, int index)
            {
                auto* dropdown = getWidget(id);
                if(dropdown) setProperty(dropdown, "currentIndex", index);
            }
            
            void qml_lua_frontend::dropdown_clear(api::ref_id id)
            {
                auto* dropdown = getWidget(id);
                if(dropdown) setProperty(dropdown, "model", QStringList{});
            }
            
            // Drawing
            void qml_lua_frontend::add_pen(api::ref_id id, const std::string& color, int width)
            {
                QVariantMap pen;
                pen["color"] = QString::fromStdString(color);
                pen["width"] = width;
                _widget_data[id] = pen;
            }
            
            void qml_lua_frontend::pen_set_width(api::ref_id id, int width)
            {
                auto& pen = _widget_data[id];
                pen["width"] = width;
            }
            
            void qml_lua_frontend::add_draw(api::ref_id id, int width, int height)
            {
                auto* canvas = createWidget("canvas", id);
                if(canvas)
                {
                    canvas->setWidth(width);
                    canvas->setHeight(height);
                }
            }
            
            void qml_lua_frontend::draw_line(api::ref_id canvas_id, api::ref_id line_id, api::ref_id pen_id, double x1, double y1, double x2, double y2)
            {
                auto* canvas = getWidget(canvas_id);
                if(!canvas) return;
                
                QVariantMap cmd;
                cmd["type"] = "line";
                cmd["x1"] = x1;
                cmd["y1"] = y1;
                cmd["x2"] = x2;
                cmd["y2"] = y2;
                
                if(_widget_data.count(pen_id))
                {
                    auto& pen = _widget_data[pen_id];
                    cmd["color"] = pen["color"];
                    cmd["width"] = pen["width"];
                }
                
                QMetaObject::invokeMethod(canvas, "addDrawCommand", Q_ARG(QVariant, cmd));
            }
            
            void qml_lua_frontend::draw_circle(api::ref_id canvas_id, api::ref_id circle_id, api::ref_id pen_id, double x, double y, double radius)
            {
                auto* canvas = getWidget(canvas_id);
                if(!canvas) return;
                
                QVariantMap cmd;
                cmd["type"] = "circle";
                cmd["x"] = x;
                cmd["y"] = y;
                cmd["radius"] = radius;
                
                if(_widget_data.count(pen_id))
                {
                    auto& pen = _widget_data[pen_id];
                    cmd["color"] = pen["color"];
                    cmd["width"] = pen["width"];
                }
                
                QMetaObject::invokeMethod(canvas, "addDrawCommand", Q_ARG(QVariant, cmd));
            }
            
            void qml_lua_frontend::draw_image(api::ref_id canvas_id, api::ref_id image_item_id, api::ref_id image_id, double x, double y, double w, double h)
            {
                // TODO: Implement image drawing
            }
            
            void qml_lua_frontend::draw_clear(api::ref_id id)
            {
                auto* canvas = getWidget(id);
                if(canvas) QMetaObject::invokeMethod(canvas, "clearDrawCommands");
            }
            
            void qml_lua_frontend::draw_line_set(api::ref_id line_id, api::ref_id pen_id, double x1, double y1, double x2, double y2)
            {
                // Store line data for later use
                QVariantMap line;
                line["x1"] = x1;
                line["y1"] = y1;
                line["x2"] = x2;
                line["y2"] = y2;
                line["pen"] = static_cast<qulonglong>(pen_id);
                _widget_data[line_id] = line;
            }
            
            void qml_lua_frontend::draw_line_set_pen(api::ref_id canvas_id, api::ref_id line_id, api::ref_id pen_id)
            {
                if(_widget_data.count(line_id))
                {
                    _widget_data[line_id]["pen"] = static_cast<qulonglong>(pen_id);
                }
            }
            
            void qml_lua_frontend::draw_circle_set(api::ref_id circle_id, api::ref_id pen_id, double x, double y, double radius)
            {
                QVariantMap circle;
                circle["x"] = x;
                circle["y"] = y;
                circle["radius"] = radius;
                circle["pen"] = static_cast<qulonglong>(pen_id);
                _widget_data[circle_id] = circle;
            }
            
            void qml_lua_frontend::draw_circle_set_pen(api::ref_id canvas_id, api::ref_id circle_id, api::ref_id pen_id)
            {
                if(_widget_data.count(circle_id))
                {
                    _widget_data[circle_id]["pen"] = static_cast<qulonglong>(pen_id);
                }
            }
            
            void qml_lua_frontend::draw_image_set(api::ref_id image_item_id, api::ref_id image_id, double x, double y, double w, double h)
            {
                // TODO: Implement image positioning
            }
            
            // Timer
            void qml_lua_frontend::add_timer(api::ref_id id, int interval)
            {
                auto* timer = createWidget("timer", id);
                if(timer)
                {
                    setProperty(timer, "interval", interval);
                }
            }
            
            bool qml_lua_frontend::timer_running(api::ref_id id)
            {
                auto* timer = getWidget(id);
                if(!timer) return false;
                return getProperty(timer, "running").toBool();
            }
            
            void qml_lua_frontend::timer_stop(api::ref_id id)
            {
                auto* timer = getWidget(id);
                if(timer) setProperty(timer, "running", false);
            }
            
            void qml_lua_frontend::timer_start(api::ref_id id)
            {
                auto* timer = getWidget(id);
                if(timer) setProperty(timer, "running", true);
            }
            
            void qml_lua_frontend::timer_set_interval(api::ref_id id, int interval)
            {
                auto* timer = getWidget(id);
                if(timer) setProperty(timer, "interval", interval);
            }
            
            // Image
            bool qml_lua_frontend::add_image(api::ref_id id, const util::bytes& data)
            {
                auto* image = createWidget("image", id);
                if(!image) return false;
                
                QImage qimg;
                if(qimg.loadFromData(reinterpret_cast<const uchar*>(data.data()), data.size()))
                {
                    // TODO: Set image source from QImage
                    return true;
                }
                return false;
            }
            
            int qml_lua_frontend::image_width(api::ref_id id)
            {
                auto* image = getWidget(id);
                if(!image) return 0;
                return getProperty(image, "sourceSize").toSize().width();
            }
            
            int qml_lua_frontend::image_height(api::ref_id id)
            {
                auto* image = getWidget(id);
                if(!image) return 0;
                return getProperty(image, "sourceSize").toSize().height();
            }
            
            // Audio
            void qml_lua_frontend::add_mic(api::ref_id id, const std::string& codec)
            {
                // TODO: Implement microphone support
            }
            
            void qml_lua_frontend::mic_start(api::ref_id id)
            {
                // TODO: Implement microphone start
            }
            
            void qml_lua_frontend::mic_stop(api::ref_id id)
            {
                // TODO: Implement microphone stop
            }
            
            void qml_lua_frontend::mic_disable()
            {
                _mic_enabled = false;
            }
            
            void qml_lua_frontend::mic_enable()
            {
                _mic_enabled = true;
            }
            
            bool qml_lua_frontend::mic_enabled() const
            {
                return _mic_enabled;
            }
            
            void qml_lua_frontend::add_speaker(api::ref_id id, const std::string& codec)
            {
                // TODO: Implement speaker support
            }
            
            void qml_lua_frontend::speaker_mute(api::ref_id id)
            {
                // TODO: Implement speaker mute
            }
            
            void qml_lua_frontend::speaker_unmute(api::ref_id id)
            {
                // TODO: Implement speaker unmute
            }
            
            void qml_lua_frontend::speaker_play(api::ref_id id, const util::bytes& data)
            {
                // TODO: Implement audio playback
            }
            
            // File operations
            api::file_data qml_lua_frontend::open_file()
            {
                QString fileName = QFileDialog::getOpenFileName(nullptr, "Open File");
                if(fileName.isEmpty()) return api::file_data{"", "", false};
                
                QFile file{fileName};
                if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
                    return api::file_data{"", "", false};
                
                QTextStream in{&file};
                QString content = in.readAll();
                
                return api::file_data{
                    fileName.toStdString(),
                    content.toStdString(),
                    true
                };
            }
            
            api::bin_file_data qml_lua_frontend::open_bin_file()
            {
                QString fileName = QFileDialog::getOpenFileName(nullptr, "Open Binary File");
                if(fileName.isEmpty()) return api::bin_file_data{"", util::bytes{}, false};
                
                QFile file{fileName};
                if(!file.open(QIODevice::ReadOnly))
                    return api::bin_file_data{"", util::bytes{}, false};
                
                QByteArray data = file.readAll();
                
                return api::bin_file_data{
                    fileName.toStdString(),
                    util::bytes{data.begin(), data.end()},
                    true
                };
            }
            
            bool qml_lua_frontend::save_file(const std::string& name, const std::string& data)
            {
                QString fileName = QFileDialog::getSaveFileName(nullptr, "Save File", QString::fromStdString(name));
                if(fileName.isEmpty()) return false;
                
                QFile file{fileName};
                if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
                    return false;
                
                QTextStream out{&file};
                out << QString::fromStdString(data);
                return true;
            }
            
            bool qml_lua_frontend::save_bin_file(const std::string& name, const util::bytes& data)
            {
                QString fileName = QFileDialog::getSaveFileName(nullptr, "Save Binary File", QString::fromStdString(name));
                if(fileName.isEmpty()) return false;
                
                QFile file{fileName};
                if(!file.open(QIODevice::WriteOnly))
                    return false;
                
                file.write(reinterpret_cast<const char*>(data.data()), data.size());
                return true;
            }
            
            // Debug/output
            void qml_lua_frontend::print(const std::string& text)
            {
                emit outputAppended(QString::fromStdString(text));
            }
            
            // GUI operations
            void qml_lua_frontend::height(int h)
            {
                if(_root_item) _root_item->setHeight(h);
            }
            
            void qml_lua_frontend::width(int w)
            {
                if(_root_item) _root_item->setWidth(w);
            }
            
            bool qml_lua_frontend::visible()
            {
                return _root_item ? _root_item->isVisible() : false;
            }
            
            void qml_lua_frontend::grow()
            {
                // TODO: Implement grow behavior
            }
            
            void qml_lua_frontend::alert()
            {
                emit alertRequested();
            }
            
            void qml_lua_frontend::report_error(const std::string& error)
            {
                emit errorOccurred(QString::fromStdString(error));
            }
            
            void qml_lua_frontend::adjust_size()
            {
                // TODO: Implement size adjustment
            }
            
            void qml_lua_frontend::reset()
            {
                // Clear all widgets
                for(auto& kv : _widgets)
                {
                    if(kv.second) kv.second->deleteLater();
                }
                _widgets.clear();
                _widget_data.clear();
            }
            
        }
    }
}