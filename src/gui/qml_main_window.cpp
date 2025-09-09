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

#include "gui/qml_main_window.hpp"
#include "gui/qml_main_controller.hpp"
#include "gui/qml_conversation_model.hpp"
#include "gui/qml_engine_holder.hpp"
#include "util/dbc.hpp"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQuickWindow>
#include <qqml.h>

namespace fire
{
    namespace gui
    {
        struct qml_main_window::Private
        {
            main_window_context context;
            std::unique_ptr<QQmlApplicationEngine> engine;
            std::unique_ptr<QmlMainController> controller;
        };
        
        qml_main_window::qml_main_window(const main_window_context& context)
            : d(std::make_unique<Private>())
        {
            REQUIRE(context.user);
            
            d->context = context;
            d->engine = std::make_unique<QQmlApplicationEngine>();
            d->controller = std::make_unique<QmlMainController>(context);
            
            // Set the engine in the singleton holder
            QmlEngineHolder::setInstance(d->engine.get());
            
            // Register QML types
            qmlRegisterType<qml_conversation_model>("Fire", 1, 0, "ConversationModel");
            
            // Expose controller to QML
            d->engine->rootContext()->setContextProperty("mainController", d->controller.get());
            
            // Load QML file
            // TODO: Use QRC resource instead of file path
            QString qmlPath = "../src/gui/qml/MainWindow.qml";
            d->engine->load(QUrl::fromLocalFile(qmlPath));
            
            // Check if loading was successful
            if (d->engine->rootObjects().isEmpty())
            {
                // Try alternative path from build directory
                qmlPath = "../../firestr/src/gui/qml/MainWindow.qml";
                d->engine->load(QUrl::fromLocalFile(qmlPath));
                
                if (d->engine->rootObjects().isEmpty())
                {
                    throw std::runtime_error("Failed to load QML main window from: " + qmlPath.toStdString());
                }
            }
        }
        
        qml_main_window::~qml_main_window()
        {
            // Cleanup is handled by unique_ptr
        }
        
        void qml_main_window::show()
        {
            REQUIRE(d);
            REQUIRE(d->engine);
            
            auto rootObjects = d->engine->rootObjects();
            if (!rootObjects.isEmpty())
            {
                if (auto window = qobject_cast<QQuickWindow*>(rootObjects.first()))
                {
                    window->show();
                }
            }
        }
        
        int qml_main_window::exec()
        {
            REQUIRE(d);
            
            show();
            return qApp->exec();
        }
    }
}