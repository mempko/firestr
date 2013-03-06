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

file(GLOB src *.cpp app/*.cpp)
file(GLOB headers *.hpp app/*.hpp)
file(GLOB forms *.ui app/*.ui)
file(GLOB resources *.qrc app/*.qrc)

#qt specific
QT4_WRAP_CPP(moc_headers ${headers} OPTIONS "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED") 
QT4_WRAP_UI(moc_forms ${forms} OPTIONS "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
QT4_ADD_RESOURCES(moc_resources ${resources} OPTIONS "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(fire_gui STATIC
    ${src}
    ${moc_headers}
    ${moc_forms}
    ${moc_resources})
