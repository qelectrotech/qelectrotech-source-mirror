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

# Issue #904 -- see run.sh for what this guards and how.
#
# Driven by gdb, so it is registered on Linux only. It reports 77 (SKIP)
# rather than failing when it cannot run at all: no gdb, a gdb without
# Python, or a stripped binary whose symbols it cannot call.
if(UNIX AND NOT APPLE)
  message(". Add test modal_quit_regression")
  add_test(
    NAME modal_quit_regression
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run.sh --binary $<TARGET_FILE:qelectrotech>)
  set_tests_properties(modal_quit_regression PROPERTIES
    SKIP_RETURN_CODE 77
    TIMEOUT 180)
endif()
