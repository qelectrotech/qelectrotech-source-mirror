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

message(" - fetch_pugixml")

Include(FetchContent)

option(BUILD_PUGIXML "Build pugixml library, use system one otherwise" YES)

if(BUILD_PUGIXML)

  # Pinned to the commit v1.15 points at, not to the tag itself: a tag is a
  # mutable pointer that the upstream owner can move, so fetching by tag means
  # a future build can silently get different code than this one did.
  FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        ee86beb30e4973f5feffe3ce63bfa4fbadf72f38) # v1.15
  set(PUGIXML_INSTALL OFF CACHE INTERNAL "")
  FetchContent_MakeAvailable(pugixml)
else()
  find_package(pugixml REQUIRED)
endif()
