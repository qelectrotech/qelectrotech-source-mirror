# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\qelectrotech_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\qelectrotech_autogen.dir\\ParseCache.txt"
  "_deps\\singleapplication-build\\CMakeFiles\\SingleApplication_autogen.dir\\AutogenUsed.txt"
  "_deps\\singleapplication-build\\CMakeFiles\\SingleApplication_autogen.dir\\ParseCache.txt"
  "_deps\\singleapplication-build\\SingleApplication_autogen"
  "qelectrotech_autogen"
  )
endif()
