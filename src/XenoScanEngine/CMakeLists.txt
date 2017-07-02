file(GLOB SCANNER_HEADER_FILES
    "Scan*.h"
)
file(GLOB SCANNER_SOURCE_FILES
    "Scan*.cpp"
)

file(GLOB HEADER_FILES
    "Assert.h"
    "FastAllocator.h"
)
file(GLOB SOURCE_FILES
    "FastAllocator.cpp"
)

add_library(XenoScanEngine STATIC ${SCANNER_HEADER_FILES} ${SCANNER_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(XenoScanEngine XenoLua)

source_group("Headers"        FILES ${HEADER_FILES})
source_group("Sources"        FILES ${SOURCE_FILES})

source_group("Headers\\Scanner"        FILES ${SCANNER_HEADER_FILES})
source_group("Sources\\Scanner"        FILES ${SCANNER_SOURCE_FILES})