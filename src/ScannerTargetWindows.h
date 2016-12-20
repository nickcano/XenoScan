#pragma once
#include "ScannerTarget.h"


class ScannerTargetWindows : public ScannerTarget
{
public:
	ScannerTargetWindows();
	~ScannerTargetWindows();

	virtual bool attach(const ProcessIdentifier &pid);
	virtual bool isAttached() const;
	virtual MemoryAddress lowestAddress() const;
	virtual MemoryAddress highestAddress() const;
	virtual size_t pageSize() const;
	virtual size_t chunkSize() const;

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo) const;
	//virtual bool isDisassemblySupported() const { return true; }

	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const;

private:
	ProcessHandle processHandle;
	MemoryAddress mainModuleStart, mainModuleEnd;
	virtual bool read(const MemoryAddress &adr, const size_t objectSize, void* result) const;

	MemoryAddress getMainModuleBaseAddress() const;
};