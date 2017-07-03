cmake_minimum_required (VERSION 2.8.11)
project (XenoScan)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}\\bin")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}\\bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}\\bin")


include_directories ("src")

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}\\deps\\luajit\\src")
link_libraries ("${CMAKE_CURRENT_SOURCE_DIR}\\deps\\luajit\\src\\lua51.lib")

link_libraries ("Psapi.lib")

add_subdirectory ("src")