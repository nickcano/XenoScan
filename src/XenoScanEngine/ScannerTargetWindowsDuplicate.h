#pragma once

#ifndef XENOSCANENGINE_LIB
#error This header is for internal library use. Include "ScannerTarget.h" instead.
#endif

#ifndef WIN32
#error This header should only be included in the Windows build.
#endif

#include "ScannerTargetWindowsBase.h"
class ScannerTargetWindowsDuplicate : public ScannerTargetWindowsBase
{
public:
	static ScannerTarget::FACTORY_TYPE::KEY_TYPE Key;
	ScannerTargetWindowsDuplicate();
	~ScannerTargetWindowsDuplicate();
protected:
	virtual ProcessHandle obtainProcessHandle(const ProcessIdentifier &pid) const;
};