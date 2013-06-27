#
# Copyright (C) 2013  Maxim Noah Khailo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include_directories(.)
include_directories(..)

file(GLOB src *.cpp)
file(GLOB headers *.hpp)
file(GLOB forms *.ui)
file(GLOB resources *.qrc)

#qt specific
QT5_WRAP_CPP(moc_headers ${headers})
QT5_WRAP_UI(moc_forms ${forms})
QT5_ADD_RESOURCES(moc_resources ${resources})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    set(PROGNAME firestr)
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1)
    set(MACOSX_BUNDLE_VERSION 0.1)
    set(MACOSX_BUNDLE_LONG_VERSION Version 0.1)
    set(MAXOSX_BUNDLE_NAME firestr)
    set(MAXOSX_BUNDLE_INFO_STRING firestr)
    set(MAXOSX_BUNDLE_COPYRIGHT "(C) 2013 Maxim Noah Khailo")

    add_executable(
        firestr
        MACOSX_BUNDLE
        ${src}
        ${moc_headers}
        ${moc_forms}
        ${moc_resources})
    add_custom_command(TARGET firestr POST_BUILD
        command mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/Resources
        command cp -p ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/MacOS
        command cp *.qm ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/Resources/)
    
        

else(APPLE)
    add_executable(
        firestr
        ${src}
        ${moc_headers}
        ${moc_forms}
        ${moc_resources})
endif(APPLE)

qt5_use_modules(firestr Widgets Network)

target_link_libraries(
    firestr
    fire_gui
    fire_session
    fire_user
    fire_messages
    fire_service
    fire_message
    fire_network
    fire_util
    fire_slb
    ${Boost_LIBRARIES}
    ${MISC_LIBRARIES})

add_dependencies(
    firestr 
    fire_gui
    fire_session
    fire_messages
    fire_user
    fire_message
    fire_network
    fire_util
    fire_slb)
