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

message(" - fetch_singleapplication")

# https://github.com/itay-grudev/SingleApplication/issues/18
#qmake
#DEFINES += QAPPLICATION_CLASS=QGuiApplication
set(QAPPLICATION_CLASS QApplication)

Include(FetchContent)

FetchContent_Declare(
  SingleApplication
  GIT_REPOSITORY https://github.com/itay-grudev/SingleApplication.git
  GIT_TAG        v3.5.1)

FetchContent_MakeAvailable(SingleApplication)
