file(GLOB SCANNER_HEADER_FILES
	"ScanResult.h"
	"ScanState.h"
	"Scanner.h"
	"ScannerTypes.h"
)
file(GLOB SCANNER_SOURCE_FILES
	"Scanner.cpp"
)

file(GLOB SCANNER_TARGET_HEADER_FILES
	"ScannerTarget.h"
	"ScannerTargetHelper.h"
)

file(GLOB SCANNER_TARGET_SOURCE_FILES
	"ScannerTarget.cpp"
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

file(GLOB SCANNER_TARGET_EMULATOR_HEADER_FILES
	"ScannerTargetDolphin.h"
)
file(GLOB SCANNER_TARGET_EMULATOR_SOURCE_FILES
	"ScannerTargetDolphin.cpp"
)

file(GLOB SCANNER_VARIANT_HEADER_FILES
	"ScanVariant.h"
	"ScanVariantTypeTraits.h"
	"ScanVariantTypeTraits.hpp"
	"ScanVariantComparator.h"
)

file(GLOB SCANNER_VARIANT_SOURCE_FILES
	"ScanVariant.cpp"
)


file(GLOB SCANNER_BLUEPRINT_HEADER_FILES
	"NativeClassInstanceBlueprint.h"
	"StdListBlueprint.h"
	"StdMapBlueprint.h"
	"DataStructureBlueprint.h"
)

file(GLOB SCANNER_BLUEPRINT_SOURCE_FILES
	"DataStructureBlueprint.cpp"
)

file(GLOB HEADER_FILES
    "Assert.h"
    "FastAllocator.h"
    "KeyedFactory.h"
	"StackJobPool.h"
)
file(GLOB SOURCE_FILES
    "FastAllocator.cpp"
)

if (MSVC)
	file(GLOB DEBUG_HELP_FILES
		"ScanVariant.natvis"
	)
else()
	file(GLOB DEBUG_HELP_FILES
	)
endif()

add_definitions(-DXENOSCANENGINE_LIB)

add_library(XenoScanEngine STATIC
	${SCANNER_HEADER_FILES}
	${SCANNER_SOURCE_FILES}

	${SCANNER_TARGET_HEADER_FILES}
	${SCANNER_TARGET_SOURCE_FILES}
	${SCANNER_TARGET_NATIVE_HEADER_FILES}
	${SCANNER_TARGET_NATIVE_SOURCE_FILES}
	${SCANNER_TARGET_EMULATOR_HEADER_FILES}
	${SCANNER_TARGET_EMULATOR_SOURCE_FILES}

	${SCANNER_VARIANT_HEADER_FILES}
	${SCANNER_VARIANT_SOURCE_FILES}

	${SCANNER_BLUEPRINT_HEADER_FILES}
	${SCANNER_BLUEPRINT_SOURCE_FILES}

	${SOURCE_FILES}
	${HEADER_FILES}

	${DEBUG_HELP_FILES}
)

target_link_libraries(XenoScanEngine XenoLua)

source_group("Headers"        FILES ${HEADER_FILES})
source_group("Sources"        FILES ${SOURCE_FILES})

source_group("Natvis"         FILES ${DEBUG_HELP_FILES})

source_group("Headers\\Scanner"               FILES ${SCANNER_HEADER_FILES})
source_group("Sources\\Scanner"               FILES ${SCANNER_SOURCE_FILES})

source_group("Headers\\ScannerTarget"           FILES ${SCANNER_TARGET_HEADER_FILES})
source_group("Sources\\ScannerTarget"           FILES ${SCANNER_TARGET_SOURCE_FILES})
source_group("Headers\\ScannerTarget\\Native"   FILES ${SCANNER_TARGET_NATIVE_HEADER_FILES})
source_group("Sources\\ScannerTarget\\Native"   FILES ${SCANNER_TARGET_NATIVE_SOURCE_FILES})
source_group("Headers\\ScannerTarget\\Emulator" FILES ${SCANNER_TARGET_EMULATOR_HEADER_FILES})
source_group("Sources\\ScannerTarget\\Emulator" FILES ${SCANNER_TARGET_EMULATOR_SOURCE_FILES})

source_group("Headers\\ScanVariant"           FILES ${SCANNER_VARIANT_HEADER_FILES})
source_group("Sources\\ScanVariant"           FILES ${SCANNER_VARIANT_SOURCE_FILES})

source_group("Headers\\Blueprints"            FILES ${SCANNER_BLUEPRINT_HEADER_FILES})
source_group("Sources\\Blueprints"            FILES ${SCANNER_BLUEPRINT_SOURCE_FILES})