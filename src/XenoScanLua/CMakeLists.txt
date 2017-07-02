file(GLOB LUA_LIB_FILES
    "lib.lua"
    "testlib.lua"
)
file(GLOB LUA_TEST_FILES
    "ScannerTest.lua"
    "TypeSystemTest.lua"
)

file(GLOB SOURCE_FILES
    "main.cpp"
)
file(GLOB SOURCE_LUA_FILES
    "LuaEngine.cpp"
    "LuaEngineExports.cpp"
)
file(GLOB SOURCE_TEST_FILES
    "BasicTest.cpp"
    "ScannerTest.cpp"
    "TestBase.cpp"
    "TestRunner.cpp"
)

file(GLOB HEADER_LUA_FILES
    "LuaEngine.h"
)
file(GLOB HEADER_TEST_FILES
    "BasicTest.h"
    "ScannerTest.h"
    "TestBase.h"
)

add_executable(XenoScanLua ${LUA_LIB_FILES} ${LUA_TEST_FILES} ${SOURCE_FILES} ${SOURCE_LUA_FILES} ${SOURCE_TEST_FILES} ${HEADER_LUA_FILES} ${HEADER_TEST_FILES})

target_link_libraries(XenoScanLua XenoLua)
target_link_libraries(XenoScanLua XenoScanEngine)

source_group("Scripts\\Libs"        FILES ${LUA_LIB_FILES})
source_group("Scripts\\Tests"       FILES ${LUA_TEST_FILES})

source_group("Sources"             FILES ${SOURCE_FILES})
source_group("Sources\\Lua"         FILES ${SOURCE_LUA_FILES})
source_group("Sources\\Tests"       FILES ${SOURCE_TEST_FILES})

source_group("Headers\\Lua"         FILES ${HEADER_LUA_FILES})
source_group("Headers\\Tests"       FILES ${HEADER_TEST_FILES})