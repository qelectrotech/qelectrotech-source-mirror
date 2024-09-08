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

message(" - developer_options")

# warn on *any* usage of deprecated APIs
add_definitions(-DQT_DEPRECATED_WARNINGS)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# disables all the APIs deprecated before Qt 6.0.0
#add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

# to enable function names and line numbers even for release builds
add_definitions(-DQT_MESSAGELOGCONTEXT)

# You can make your code warn on compile time for the TODO's
# In order to do so, uncomment the following line.
#add_definitions(-DTODO_LIST)

# Build with KF6
option(BUILD_WITH_KF6 "Build with KF6" ON)
