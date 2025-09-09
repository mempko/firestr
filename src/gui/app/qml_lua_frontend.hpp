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

#ifndef FIRESTR_GUI_APP_QML_LUA_FRONTEND_H
#define FIRESTR_GUI_APP_QML_LUA_FRONTEND_H

#include "gui/api/service.hpp"
#include "util/bytes.hpp"

#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QString>
#include <QVariantMap>

#include <memory>
#include <unordered_map>

namespace fire
{
    namespace gui
    {
        namespace app
        {
            class qml_lua_frontend : public QObject, public api::frontend
            {
                Q_OBJECT
                Q_PROPERTY(QQuickItem* rootItem READ rootItem NOTIFY rootItemChanged)
                
            public:
                qml_lua_frontend(QQmlEngine* engine, QObject* parent = nullptr);
                ~qml_lua_frontend();
                
                void initialize();
                
                QQuickItem* rootItem() const { return _root_item; }
                
                // Widget management
                virtual void place(api::ref_id id, int r, int c) override;
                virtual void place_across(api::ref_id id, int r, int c, int row_span, int col_span) override;
                virtual void widget_enable(api::ref_id, bool) override;
                virtual bool is_widget_enabled(api::ref_id) override;
                virtual void widget_visible(api::ref_id, bool) override;
                virtual bool is_widget_visible(api::ref_id) override;
                virtual void widget_set_style(api::ref_id, const std::string&) override;
                
                // Grid
                virtual void add_grid(api::ref_id) override;
                virtual void grid_place(api::ref_id grid_id, api::ref_id widget_id, int r, int c) override;
                virtual void grid_place_across(api::ref_id grid_id, api::ref_id widget_id, int r, int c, int row_span, int col_span) override;
                
                // Button
                virtual void add_button(api::ref_id, const std::string&) override;
                virtual std::string button_get_text(api::ref_id) override;
                virtual void button_set_text(api::ref_id, const std::string&) override;
                virtual void button_set_image(api::ref_id, api::ref_id) override;
                
                // Label
                virtual void add_label(api::ref_id, const std::string&) override;
                virtual std::string label_get_text(api::ref_id) override;
                virtual void label_set_text(api::ref_id, const std::string&) override;
                
                // Edit
                virtual void add_edit(api::ref_id, const std::string&) override;
                virtual std::string edit_get_text(api::ref_id) override;
                virtual void edit_set_text(api::ref_id, const std::string&) override;
                
                // Text Edit
                virtual void add_text_edit(api::ref_id, const std::string&) override;
                virtual std::string text_edit_get_text(api::ref_id) override;
                virtual void text_edit_set_text(api::ref_id, const std::string&) override;
                
                // List
                virtual void add_list(api::ref_id) override;
                virtual void list_add(api::ref_id list_id, api::ref_id widget_id) override;
                virtual void list_remove(api::ref_id list_id, api::ref_id widget_id) override;
                virtual size_t list_size(api::ref_id) override;
                virtual void list_clear(api::ref_id) override;
                
                // Dropdown
                virtual void add_dropdown(api::ref_id) override;
                virtual size_t dropdown_size(api::ref_id) override;
                virtual void dropdown_add_item(api::ref_id, const std::string&) override;
                virtual std::string dropdown_get_item(api::ref_id, int) override;
                virtual int dropdown_get_selected(api::ref_id) override;
                virtual void dropdown_select(api::ref_id, int) override;
                virtual void dropdown_clear(api::ref_id) override;
                
                // Drawing (simplified for now)
                virtual void add_pen(api::ref_id, const std::string&, int) override;
                virtual void pen_set_width(api::ref_id, int) override;
                virtual void add_draw(api::ref_id, int, int) override;
                virtual void draw_line(api::ref_id, api::ref_id, api::ref_id, double, double, double, double) override;
                virtual void draw_circle(api::ref_id, api::ref_id, api::ref_id, double, double, double) override;
                virtual void draw_image(api::ref_id, api::ref_id, api::ref_id, double, double, double, double) override;
                virtual void draw_clear(api::ref_id) override;
                virtual void draw_line_set(api::ref_id, api::ref_id, double, double, double, double) override;
                virtual void draw_line_set_pen(api::ref_id, api::ref_id, api::ref_id) override;
                virtual void draw_circle_set(api::ref_id, api::ref_id, double, double, double) override;
                virtual void draw_circle_set_pen(api::ref_id, api::ref_id, api::ref_id) override;
                virtual void draw_image_set(api::ref_id, api::ref_id, double, double, double, double) override;
                
                // Timer
                virtual void add_timer(api::ref_id, int) override;
                virtual bool timer_running(api::ref_id) override;
                virtual void timer_stop(api::ref_id) override;
                virtual void timer_start(api::ref_id) override;
                virtual void timer_set_interval(api::ref_id, int) override;
                
                // Image
                virtual bool add_image(api::ref_id, const util::bytes&) override;
                virtual int image_width(api::ref_id) override;
                virtual int image_height(api::ref_id) override;
                
                // Audio (simplified for now)
                virtual void add_mic(api::ref_id, const std::string&) override;
                virtual void mic_start(api::ref_id) override;
                virtual void mic_stop(api::ref_id) override;
                virtual void mic_disable() override;
                virtual void mic_enable() override;
                virtual bool mic_enabled() const override;
                virtual void add_speaker(api::ref_id, const std::string&) override;
                virtual void speaker_mute(api::ref_id) override;
                virtual void speaker_unmute(api::ref_id) override;
                virtual void speaker_play(api::ref_id, const util::bytes&) override;
                
                // File operations
                virtual api::file_data open_file() override;
                virtual api::bin_file_data open_bin_file() override;
                virtual bool save_file(const std::string&, const std::string&) override;
                virtual bool save_bin_file(const std::string&, const util::bytes&) override;
                
                // Debug/output
                virtual void print(const std::string&) override;
                
                // GUI operations
                virtual void height(int) override;
                virtual void width(int) override;
                virtual bool visible() override;
                virtual void grow() override;
                virtual void alert() override;
                virtual void report_error(const std::string&) override;
                virtual void adjust_size() override;
                virtual void reset() override;
                
            signals:
                void rootItemChanged();
                void outputAppended(const QString& text);
                void errorOccurred(const QString& error);
                void alertRequested();
                
                // Widget event signals (from widgets to Lua)
                void buttonClicked(int id);
                void editChanged(int id);
                void editFinished(int id);
                void textEditChanged(int id);
                void dropdownSelected(int id, int index);
                void timerTriggered(int id);
                
                // Widget creation signals (from backend thread to main thread)
                void requestAddButton(quint64 id, const QString& text);
                void requestAddLabel(quint64 id, const QString& text);
                void requestAddEdit(quint64 id, const QString& text);
                void requestAddTextEdit(quint64 id, const QString& text);
                void requestAddList(quint64 id);
                void requestAddGrid(quint64 id);
                void requestPlace(quint64 id, int r, int c);
                void requestPlaceAcross(quint64 id, int r, int c, int rowSpan, int colSpan);
                
                // Initialization signal
                void requestInitialize();
            
            private slots:
                void handleInitialize();
                // Slots to handle widget creation on main thread
                void handleAddButton(quint64 id, const QString& text);
                void handleAddLabel(quint64 id, const QString& text);
                void handleAddEdit(quint64 id, const QString& text);
                void handleAddTextEdit(quint64 id, const QString& text);
                void handleAddList(quint64 id);
                void handleAddGrid(quint64 id);
                void handlePlace(quint64 id, int r, int c);
                void handlePlaceAcross(quint64 id, int r, int c, int rowSpan, int colSpan);
                
            private:
                QQuickItem* createWidget(const QString& type, api::ref_id id);
                QQuickItem* getWidget(api::ref_id id);
                void setProperty(QQuickItem* item, const QString& property, const QVariant& value);
                QVariant getProperty(QQuickItem* item, const QString& property);
                
            private:
                QQmlEngine* _engine;
                QQuickItem* _root_item;
                QQmlComponent* _grid_component;
                QQmlComponent* _button_component;
                QQmlComponent* _label_component;
                QQmlComponent* _edit_component;
                QQmlComponent* _text_edit_component;
                QQmlComponent* _list_component;
                QQmlComponent* _dropdown_component;
                QQmlComponent* _canvas_component;
                QQmlComponent* _timer_component;
                QQmlComponent* _image_component;
                
                std::unordered_map<api::ref_id, QQuickItem*> _widgets;
                std::unordered_map<api::ref_id, QVariantMap> _widget_data;
                
                bool _mic_enabled;
                bool _initialized;
            };
        }
    }
}

#endif