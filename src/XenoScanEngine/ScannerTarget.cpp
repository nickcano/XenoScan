#include "ScannerTarget.h"

#include "ScannerTargetWindows.h"

CREATE_FACTORY(ScannerTarget);
CREATE_PRODUCER(ScannerTarget, NativeScannerTarget, "proc");