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

#use C++11
ADD_DEFINITIONS(-std=c++0x)

if(APPLE)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
    link_directories(/usr/local/lib)
    include_directories(/usr/local/include)
endif()

#setup boost
set(Boost_USE_STATIC_LIBS on)
find_package(Boost COMPONENTS system program_options filesystem REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

#setup QT
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

#setup misc
if(APPLE)
    set(MISC_LIBRARIES libuuid.dylib libpthread.dylib)
else()
    set(MISC_LIBRARIES libuuid.so libpthread.so librt.so)
endif()

add_subdirectory(src)
