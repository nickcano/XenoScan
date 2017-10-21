#include "ScannerTarget.h"

// We do everything in this file, rather than
// create each producer in the .cpp file of it's class,
// to ensure that the factory has already been initialized
CREATE_FACTORY(ScannerTarget);

#ifdef WIN32 
#include "ScannerTargetWindowsStandard.h"
#include "ScannerTargetWindowsDuplicate.h"

CREATE_PRODUCER(ScannerTarget, ScannerTargetWindowsStandard,   "proc");
CREATE_PRODUCER(ScannerTarget, ScannerTargetWindowsDuplicate,  "protected_proc");
#endif


#include "ScannerTargetDolphin.h"
CREATE_PRODUCER(ScannerTarget, ScannerTargetDolphin,  "dolphin");