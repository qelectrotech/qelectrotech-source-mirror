# Copyright 2006-2026 The QElectroTech Team
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

message(" - version file generation")

# Define the target generated output path
set(GENERATED_QETVERSION_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/qetversion.cpp")

# Clean up any legacy configure_file calls in the main scope to avoid race conditions.
# Instead, create a script that will execute on EVERY build step.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/RunGitCheck.cmake" "
  find_package(Git QUIET)
  set(GIT_COMMIT_SHA \"unknown\")

  if(GIT_FOUND AND EXISTS \"${PROJECT_SOURCE_DIR}/.git\")
    execute_process(
      COMMAND \${GIT_EXECUTABLE} -C \"${PROJECT_SOURCE_DIR}\" rev-parse --verify HEAD
      OUTPUT_VARIABLE GIT_COMMIT_SHA
      RESULT_VARIABLE GIT_COMMIT_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_COMMIT_RESULT EQUAL 0)
      set(GIT_COMMIT_SHA \"unknown\")
    endif()
  endif()

  # Pass along configuration variables inherited from main project
  set(QET_VERSION_TYPE \"${QET_VERSION_TYPE}\")
  set(PROJECT_VERSION_MAJOR \"${PROJECT_VERSION_MAJOR}\")
  set(PROJECT_VERSION_MINOR \"${PROJECT_VERSION_MINOR}\")
  set(PROJECT_VERSION_PATCH \"${PROJECT_VERSION_PATCH}\")

  # Smart-update the file only if the SHA or variables actually change
  configure_file(
    \"${PROJECT_SOURCE_DIR}/sources/qetversion.cpp.in\"
    \"${GENERATED_QETVERSION_CPP}\"
    @ONLY
  )
")

# Build target to force script execution before the main project compiles
add_custom_target(
  GenerateVersionFile ALL
  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/RunGitCheck.cmake"
  BYPRODUCTS "${GENERATED_QETVERSION_CPP}"
  COMMENT "Checking Git repository for latest commit SHA..."
  VERBATIM
)
