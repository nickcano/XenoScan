cmake_minimum_required (VERSION 2.8.11)
project (XenoScan)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")


include_directories ("src")

include_directories ("deps/luajit-2.0/src")
link_libraries ("deps/luajit-2.0/lua51")

add_subdirectory ("src")
add_subdirectory ("deps")