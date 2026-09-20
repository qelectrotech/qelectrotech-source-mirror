# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)

add_library(sqlite3 STATIC
  src/sqlite3.c
)

add_library(SQLite3::SQLite3 ALIAS sqlite3)

target_include_directories(sqlite3
  PUBLIC
    include
)

target_compile_features(sqlite3 PUBLIC c_std_99)

target_compile_definitions(sqlite3 PRIVATE
  SQLITE_THREADSAFE=1
  SQLITE_DEFAULT_MEMSTATUS=0
  SQLITE_LIKE_DOESNT_MATCH_BLOBS
  SQLITE_OMIT_SHARED_CACHE
)

set_target_properties(sqlite3 PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
