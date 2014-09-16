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
# In addition, as a special exception, the copyright holders give 
# permission to link the code of portions of this program with the 
# OpenSSL library under certain conditions as described in each 
# individual source file, and distribute linked combinations 
# including the two.
#
# You must obey the GNU General Public License in all respects for 
# all of the code used other than OpenSSL. If you modify file(s) with 
# this exception, you may extend this exception to your version of the 
# file(s), but you are not obligated to do so. If you do not wish to do 
# so, delete this exception statement from your version. If you delete 
# this exception statement from all source files in the program, then 
# also delete it here.

#use C++11
ADD_DEFINITIONS(-std=c++11)

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
    set(plugin_dest_dir bin)
    set(MACISX_BUNDLE_NAME "Firestr")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.mempko.firestr")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1)
    set(MACOSX_BUNDLE_VERSION 0.1)
    set(MACOSX_BUNDLE_LONG_VERSION Version 0.1)
    set(MACOSX_BUNDLE_NAME firestr)
    set(MACOSX_BUNDLE_INFO_STRING firestr)
    set(MACOSX_BUNDLE_COPYRIGHT "(C) 2013 Maxim Noah Khailo")

    add_executable(
        firestr
        #MACOSX_BUNDLE
        ${src}
        ${moc_headers}
        ${moc_forms}
        ${moc_resources})
    add_custom_command(TARGET firestr PRE_BUILD
        command mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/Resources
        command cp -p ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/MacOS
        command cp *.qm ${CMAKE_CURRENT_BINARY_DIR}/firestr.app/Contents/Resources/)

    set_target_properties(firestr PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/mac_bundle.plist.in)

    set(APPS ${CMAKE_CURRENT_BINARY_DIR}/firestr.app)

    install(CODE "
        file(GLOB_RECURSE QTPLUGINS
            \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${QT_LIBRARY_DIRS}\")
        " COMPONENT Runtime)

    set(CPACK_BINARY_DRAGNDROP ON)
    set(CPACK_BUNDLE_NAME Firestr)
    set(CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/cmake/mac_bundle.plist.in)
    set(CPACK_GENERATOR Bundle)
    include(CPack)

elseif(CMAKE_HOST_WIN32)
    add_executable(
        firestr
        ${src}
        ${moc_headers}
        ${moc_forms}
        ${moc_resources})
else()
    add_executable(
        firestr
        ${src}
        ${moc_headers}
        ${moc_forms}
        ${moc_resources})
endif()

qt5_use_modules(firestr Widgets Network Multimedia MultimediaWidgets)

target_link_libraries(
    firestr
    fire_gui
    fire_conversation
    fire_user
    fire_messages
    fire_service
    fire_message
    fire_network
    fire_security
    fire_util
    fire_slb
    ${Boost_LIBRARIES}
    ${MISC_LIBRARIES})

add_dependencies(
    firestr 
    fire_gui
    fire_conversation
    fire_messages
    fire_user
    fire_message
    fire_network
    fire_security
    fire_util
    fire_slb)

install(TARGETS firestr DESTINATION bin)
