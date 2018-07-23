
file(GLOB SOURCE_FILES
    "*.cpp"
)
file(GLOB HEADER_FILES
    "*.h"
)

add_library(XenoLua STATIC ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET XenoLua PROPERTY CXX_STANDARD 17)
set_property(TARGET XenoLua PROPERTY CXX_STANDARD_REQUIRED ON)

source_group("Sources"        FILES ${SOURCE_FILES})
source_group("Headers"        FILES ${HEADER_FILES})