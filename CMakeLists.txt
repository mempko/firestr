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

cmake_minimum_required(VERSION 2.6)
project(firestr)

if(APPLE)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
    link_directories(/usr/local/lib)
    include_directories(/usr/local/include)

    #botan from macports
    include_directories(/usr/local/include/botan-1.10/)
    link_directories(/usr/local/lib/)
elseif(CMAKE_HOST_WIN32)
    link_directories(C:/botan/lib)
    link_directories(C:/Boost/lib)
    include_directories(C:/botan/include)
    include_directories(C:/Boost/include/boost_1_55_0)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    set(CMAKE_CXX_FLAGS "/EHsc")

else()
    #botan
    include_directories(/usr/include/botan-1.10/)
endif()

#setup boost
set(Boost_USE_STATIC_LIBS on)
find_package(Boost COMPONENTS system program_options filesystem regex thread REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

#setup QT
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Multimedia)
find_package(Qt5MultimediaWidgets)
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5Multimedia_INCLUDE_DIRS})
include_directories(${Qt5MultimediaWidgets_INCLUDE_DIRS})
add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5Widgets_DEFINITIONS})
add_definitions(${Qt5Network_DEFINITIONS})
add_definitions(${Qt5Multimedia_DEFINITIONS})
add_definitions(${Qt5MultimediaWidgets_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

if(APPLE)
    set(MISC_LIBRARIES libopus.a libuuid.dylib libpthread.dylib libbotan-1.10.a)
elseif(CMAKE_HOST_WIN32)
    set(MISC_LIBRARIES botan.lib)
else()
    set(MISC_LIBRARIES libopus.a libuuid.so libpthread.so librt.so libbotan-1.10.a libcrypto.so libgmp.so libsnappy.a)
endif()

add_subdirectory(src)
