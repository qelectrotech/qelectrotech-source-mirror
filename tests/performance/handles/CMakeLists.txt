cmake_minimum_required(VERSION 3.16)
project(handle_refresh_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(handle_refresh_probe main.cpp
  ${QET_SOURCE}/sources/QetGraphicsItemModeler/qetgraphicshandleritem.cpp
  ${QET_SOURCE}/sources/QetGraphicsItemModeler/qetgraphicshandleritem.h)
target_include_directories(handle_refresh_probe PRIVATE ${QET_SOURCE}/sources)
target_link_libraries(handle_refresh_probe PRIVATE Qt6::Widgets)
if(WIN32)
 target_link_libraries(handle_refresh_probe PRIVATE psapi)
endif()
