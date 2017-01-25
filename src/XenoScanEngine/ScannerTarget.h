#pragma once
#include <memory>

#include "ScannerTypes.h"

class ScannerTarget
{
public:
	ScannerTarget() {};
	~ScannerTarget() {};

	virtual bool attach(const ProcessIdentifier &pid) = 0;
	virtual bool isAttached() const = 0;
	virtual MemoryAddress lowestAddress() const = 0;
	virtual MemoryAddress highestAddress() const = 0;
	virtual size_t pageSize() const = 0;
	virtual size_t chunkSize() const = 0;

	template<typename T>
	T read(const MemoryAddress &adr) const
	{
		T ret;
		memset(&ret, 0, sizeof(T));
		this->read(adr, sizeof(T), &ret);
		return ret;
	}

	template<typename T>
	bool read(const MemoryAddress &adr, T &value) const
	{
		return this->read(adr, sizeof(T), &value);
	}

	template<typename T>
	bool readArray(const MemoryAddress &adr, const size_t size, T* &result) const
	{
		return this->read(adr, sizeof(T) * size, &result[0]);
	}

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo) const = 0;
	//virtual bool isDisassemblySupported() const { return false; }

	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const = 0;

	//TODO: clean this up
	static std::shared_ptr<ScannerTarget> createScannerTarget();

private:
	virtual bool read(const MemoryAddress &adr, const size_t objectSize, void* result) const = 0;

};

typedef std::shared_ptr<ScannerTarget> ScannerTargetShPtr;