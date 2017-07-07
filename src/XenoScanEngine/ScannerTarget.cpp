#include "ScannerTarget.h"

#include "ScannerTargetWindows.h"

// We do everything in this file, rather than
// create each producer in the .cpp file of it's class,
// to ensure that the factory has already been initialized
CREATE_FACTORY(ScannerTarget);
CREATE_PRODUCER(ScannerTarget, NativeScannerTarget, "proc");