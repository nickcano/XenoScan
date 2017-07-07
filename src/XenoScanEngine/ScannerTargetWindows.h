#pragma once
#include "ScannerTarget.h"


class ScannerTargetWindows : public ScannerTarget
{
public:
	ScannerTargetWindows();
	~ScannerTargetWindows();

	virtual bool attach(const ProcessIdentifier &pid);
	virtual bool isAttached() const;

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const;
	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const;

protected:
	virtual bool rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const;
	virtual bool rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const;

private:
	ProcessHandle processHandle;
	MemoryAddress mainModuleStart, mainModuleEnd;
	size_t pageSize;

	MemoryAddress getMainModuleBaseAddress() const;
};