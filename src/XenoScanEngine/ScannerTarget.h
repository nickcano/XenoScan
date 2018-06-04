#pragma once
#include <memory>
#include <set>

#include "ScannerTargetHelper.h"
#include "ScannerTypes.h"

class ScannerTarget : public ScannerTargetHelper<ScannerTarget>
{
friend class ScannerTargetHelper<ScannerTarget>;
public:
	static FACTORY_TYPE Factory;

	ScannerTarget() {};
	~ScannerTarget() {};

	// Now, everything below this point is abstract
	virtual bool attach(const ProcessIdentifier &pid) = 0;
	virtual bool isAttached() const = 0;

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const = 0;
	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const = 0;

	virtual uint64_t getFileTime64() const = 0;
	virtual uint32_t getTickTime32() const = 0;

protected:
	bool littleEndian;
	size_t pointerSize;
	MemoryAddress lowestAddress, highestAddress;
	std::set<std::string> supportedBlueprints;

	virtual bool rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const = 0;
	virtual bool rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const = 0;
};

typedef std::shared_ptr<ScannerTarget> ScannerTargetShPtr;