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

#qt specific
QT5_WRAP_CPP(moc_headers ${headers})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(
    firegreet
    ${src}
    ${moc_headers})

qt5_use_modules(firegreet Widgets Network)

target_link_libraries(
    firegreet
    fire_user
    fire_messages
    fire_service
    fire_message
    fire_network
    fire_util
    ${Boost_LIBRARIES}
    ${MISC_LIBRARIES})

add_dependencies(
    firegreet 
    fire_messages
    fire_user
    fire_message
    fire_network
    fire_util)

install(TARGETS firegreet DESTINATION bin)
