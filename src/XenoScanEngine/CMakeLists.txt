file(GLOB SCANNER_HEADER_FILES
	"ScanResult.h"
	"ScanState.h"
	"Scanner.h"
	"ScannerDataStructureBlueprint.h"
	"ScannerTypes.h"
)
file(GLOB SCANNER_SOURCE_FILES
	"Scanner.cpp"
	"ScannerDataStructureBlueprint.cpp"

)

file(GLOB SCANNER_TARGET_HEADER_FILES
	"ScannerTarget.h"
	"ScannerTargetHelper.h"
)

if (WIN32)
	file(GLOB SCANNER_TARGET_NATIVE_HEADER_FILES
		"ScannerTargetWindows.h"
	)
	file(GLOB SCANNER_TARGET_NATIVE_SOURCE_FILES
		"ScannerTargetWindows.cpp"
	)
else()

endif()

file(GLOB SCANNER_VARIANT_HEADER_FILES
	"ScanVariant.h"
	"ScanVariantTypeTraits.h"
	"ScanVariantComparator.h"
)

file(GLOB SCANNER_VARIANT_SOURCE_FILES
	"ScanVariant.cpp"
	"ScanVariantTypeTraits.cpp"
)


file(GLOB HEADER_FILES
    "Assert.h"
    "FastAllocator.h"
)
file(GLOB SOURCE_FILES
    "FastAllocator.cpp"
)

add_library(XenoScanEngine STATIC
	${SCANNER_HEADER_FILES}
	${SCANNER_SOURCE_FILES}

	${SCANNER_TARGET_HEADER_FILES}
	${SCANNER_TARGET_NATIVE_HEADER_FILES}
	${SCANNER_TARGET_NATIVE_SOURCE_FILES}

	${SCANNER_VARIANT_HEADER_FILES}
	${SCANNER_VARIANT_SOURCE_FILES}

	${SOURCE_FILES}
	${HEADER_FILES}
)

target_link_libraries(XenoScanEngine XenoLua)

source_group("Headers"        FILES ${HEADER_FILES})
source_group("Sources"        FILES ${SOURCE_FILES})

source_group("Headers\\Scanner"               FILES ${SCANNER_HEADER_FILES})
source_group("Sources\\Scanner"               FILES ${SCANNER_SOURCE_FILES})

source_group("Headers\\ScannerTarget"         FILES ${SCANNER_TARGET_HEADER_FILES})
source_group("Headers\\ScannerTarget\\Native" FILES ${SCANNER_TARGET_NATIVE_HEADER_FILES})
source_group("Sources\\ScannerTarget\\Native" FILES ${SCANNER_TARGET_NATIVE_SOURCE_FILES})

source_group("Headers\\ScanVariant"           FILES ${SCANNER_VARIANT_HEADER_FILES})
source_group("Sources\\ScanVariant"           FILES ${SCANNER_VARIANT_SOURCE_FILES})