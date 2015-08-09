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

#use C++11
ADD_DEFINITIONS(-std=c++11)

include_directories(.)
include_directories(..)

file(GLOB src *.cpp)
file(GLOB headers *.hpp)

#qt specific
QT5_WRAP_CPP(moc_headers ${headers})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(
    fireperf
    ${src}
    ${moc_headers})

qt5_use_modules(fireperf Widgets Network Multimedia)

target_link_libraries(
    fireperf
    fire_user
    fire_messages
    fire_service
    fire_message
    fire_network
    fire_security
    fire_util
    ${Boost_LIBRARIES}
    ${MISC_LIBRARIES})

add_dependencies(
    fireperf 
    fire_messages
    fire_user
    fire_message
    fire_network
    fire_security
    fire_util)

install(TARGETS fireperf DESTINATION bin)
