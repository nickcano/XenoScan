cmake_minimum_required (VERSION 3.6)
project (XenoScan)

# setup some required paths
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} UOUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}" )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}" )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
    add_definitions(-DWIN64)
endif()


# set up deps and linkers and include stuff
include_directories ("src")

set (DEPS_LUAJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/luajit/src")
include_directories ("${DEPS_LUAJIT_DIR}/")
link_libraries ("${DEPS_LUAJIT_DIR}/lua51.lib")

if (WIN32)
	link_libraries ("Psapi.lib")
endif()


# include our sub projects
add_subdirectory ("src")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT XenoScanLua)


# copy the outputs from deps
add_custom_target(
    LuaJitCopyOutput ALL
)

set(LUAJIT_BINARY "${DEPS_LUAJIT_DIR}/lua51.dll")
add_custom_command(
    TARGET LuaJitCopyOutput PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LUAJIT_BINARY} $<TARGET_FILE_DIR:XenoLua>
    DEPENDS ${LUAJIT_BINARY}
    VERBATIM
)

add_custom_command(
    TARGET LuaJitCopyOutput PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:XenoLua>/jit"
)
file(GLOB LUAJIT_SCRIPTS "${DEPS_LUAJIT_DIR}/jit/*.lua")
foreach(LUAJIT_SCRIPT ${LUAJIT_SCRIPTS})
    add_custom_command(
        TARGET LuaJitCopyOutput PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LUAJIT_SCRIPT} "$<TARGET_FILE_DIR:XenoLua>/jit/"
        DEPENDS ${LUAJIT_SCRIPT}
        VERBATIM
    )
endforeach()