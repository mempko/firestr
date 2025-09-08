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

#include "gui/qml_setup.hpp"
#include "gui/qml_login_controller.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QEventLoop>

namespace us = fire::user;

namespace fire
{
    namespace gui
    {
        qml_setup_info qml_setup_user(const std::string& home)
        {
            REQUIRE_FALSE(home.empty());
            
            // Create the controller
            QmlLoginController controller(home);
            
            // Create QML engine
            QQmlApplicationEngine engine;
            
            // Register the controller with QML
            engine.rootContext()->setContextProperty("loginController", &controller);
            
            // Create event loop to wait for the dialog result
            QEventLoop loop;
            
            // Connect signals to know when to exit the loop
            QObject::connect(&controller, &QmlLoginController::loginSuccessful, 
                           &loop, &QEventLoop::quit);
            QObject::connect(&controller, &QmlLoginController::cancelled, 
                           &loop, &QEventLoop::quit);
            
            // Load the QML file from file system for now
            // TODO: Fix resource loading
            engine.load(QUrl::fromLocalFile("/home/mempko/projects/firestr/src/gui/qml/LoginScreen.qml"));
            
            if (engine.rootObjects().isEmpty())
            {
                LOG << "Failed to load QML login screen" << std::endl;
                return qml_setup_info{us::local_user_ptr{}, false};
            }
            
            // Get the window and show it
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (window)
            {
                window->show();
            }
            
            // Run the event loop
            loop.exec();
            
            // Close the window
            if (window)
            {
                window->close();
            }
            
            // Return the result
            bool is_new = controller.isNewUser();
            auto user = controller.getUser();
            
            return qml_setup_info{user, is_new};
        }
    }
}