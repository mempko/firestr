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

#ifndef FIRESTR_GUI_QML_SETUP_H
#define FIRESTR_GUI_QML_SETUP_H

#include "user/user.hpp"

#include <string>
#include <utility>

namespace fire
{
    namespace gui
    {
        using qml_setup_info = std::pair<user::local_user_ptr, bool>;
        
        // Main function to setup user using QML interface
        qml_setup_info qml_setup_user(const std::string& home);
    }
}

#endif // FIRESTR_GUI_QML_SETUP_H