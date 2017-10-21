#pragma once

#ifndef XENOSCANENGINE_LIB
#error This header is for internal library use. Include "ScannerTarget.h" instead.
#endif

#ifndef WIN32
#error This header should only be included in the Windows build.
#endif

#include "ScannerTarget.h"
// This is the core implementation for the Windows scanner target.
// However, we have two flavors:
//     - ScannerTargetWindowsStandard is what you'd expect, it attaches
//       to a process using OpenProcess.
//     - ScannerTargetWindowsDuplicate is different. To bypass certain
//       anti-cheat frameworks which block OpenProcess, it copies
//       the handle out of the lsass.exe process.
class ScannerTargetWindowsBase : public ScannerTarget
{
public:
	ScannerTargetWindowsBase();
	~ScannerTargetWindowsBase();

	virtual bool attach(const ProcessIdentifier &pid);
	virtual bool isAttached() const;

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const;
	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const;

protected:
	virtual bool rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const;
	virtual bool rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const;

	virtual ProcessHandle obtainProcessHandle(const ProcessIdentifier &pid) const = 0;

private:
	ProcessHandle processHandle;
	MemoryAddress mainModuleStart, mainModuleEnd;
	size_t pageSize;

	MemoryAddress getMainModuleBaseAddress() const;
};