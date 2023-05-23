#
# Copyright (C) 2015  Maxim Noah Khailo
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

cmake_minimum_required(VERSION 3.22.1)
project(firestr)

if(APPLE)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
    link_directories(/usr/local/lib)
    include_directories(/usr/local/include)

    #botan from macports
    include_directories(/usr/local/include/botan-2/)
elseif(CMAKE_HOST_WIN32)
    link_directories(C:/snappy)
    link_directories(C:/opus/lib)
    link_directories(C:/Boost/lib)
    link_directories(C:/botan/lib)
    include_directories(C:/snappy)
    include_directories(C:/opus/include)
    include_directories(C:/botan/include)
    include_directories(C:/Boost/include/boost-1_59)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    set(CMAKE_CXX_FLAGS "/EHsc")
else()
    #botan
    add_definitions(-Wall -Werror -fPIC)
    include_directories(/usr/include/botan-2/)
endif()

#setup boost
set(Boost_USE_STATIC_LIBS on)
find_package(Boost COMPONENTS system program_options filesystem regex thread REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

#setup QT
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia Core5Compat)
#find_package(Qt6Core)
#find_package(Qt6Widgets)
#find_package(Qt6Network)
#find_package(Qt6Multimedia)
include_directories(${Qt6Core_INCLUDE_DIRS})
include_directories(${Qt6Widgets_INCLUDE_DIRS})
include_directories(${Qt6Network_INCLUDE_DIRS})
include_directories(${Qt6Multimedia_INCLUDE_DIRS})
include_directories(${Qt6Core5Compat_INCLUDE_DIRS})
add_definitions(${Qt6Core_DEFINITIONS})
add_definitions(${Qt6Widgets_DEFINITIONS})
add_definitions(${Qt6Network_DEFINITIONS})
add_definitions(${Qt6Multimedia_DEFINITIONS})
add_definitions(${Qt6Core5Compat_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${Qt6Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(ICON ${PROJECT_SOURCE_DIR}/src/gui/resources/firestr.svg)
set(DESKTOP ${PROJECT_SOURCE_DIR}/debian/firestr.desktop)
if(APPLE)
    find_library(IOKIT NAMES IOKit)
    find_library(CORE NAMES CoreFoundation)
    find_library(CORE_SERVICES NAMES CoreServices)
    set(MISC_LIBRARIES /usr/local/lib/libopus.a /usr/local/lib/libuuid.dylib libpthread.dylib /usr/local/lib/libbotan-2.a /usr/local/lib/libsnappy.a ${IOKIT} ${CORE} ${CORE_SERVICES})
elseif(CMAKE_HOST_WIN32)
    set(MISC_LIBRARIES celt.lib silk_common.lib silk_float.lib opus.lib botan.lib snappy.lib)
else()
    set(MISC_LIBRARIES libopus.so libuuid.so libpthread.so librt.so libbotan-2.so libcrypto.so libgmp.so libsnappy.so libXss.so libX11.so)
endif()

add_subdirectory(src)

if(APPLE)
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Maxim Noah Khailo <firestr.dev@gmail.com>") 
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), libgcc1 (>= 1:4.1.1), libgmp10, libssl1.0.0 (>= 1.0.0), libstdc++6 (>= 4.8.1), libqt6core5, libqt6widgets6, libqt6network6")
    set(CPACK_DEBIAN_PACKAGE_LICENSE "GPLv3")
    set(CPACK_PACKAGE_CONTACT "firestr.dev@gmail.com")
    set(CPACK_PACKAGE_VERSION "0.2")
    set(CPACK_PACKAGE_DESCRIPTION "A simple platform for p2p computation and communication.")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple platform fo p2p computation and communication.")
    set(CPACK_PACKAGE_SOURCE "")
    SET (CPACK_COMPONENTS_ALL Libraries ApplicationData)
    
    include(CPack)
endif()
 
