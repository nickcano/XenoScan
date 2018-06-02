#pragma once

#ifndef XENOSCANENGINE_LIB
#error This header is for internal library use. Include "ScannerTarget.h" instead.
#endif

#include <vector>

#include "ScannerTarget.h"

class ScannerTargetDolphin : public ScannerTarget
{
public:
	static ScannerTarget::FACTORY_TYPE::KEY_TYPE Key;

	ScannerTargetDolphin();
	~ScannerTargetDolphin();

	virtual bool attach(const ProcessIdentifier &pid);
	virtual bool isAttached() const;

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const;
	virtual bool getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const;

	virtual uint64_t getFileTime64() const;
	virtual uint32_t getTickTime32() const;

protected:
	virtual bool rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const;
	virtual bool rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const;

private:
	void* sharedMemoryHandle;

	static const MemoryMapEntry Mem1CachedMap;
	static const MemoryMapEntry Mem1UncachedMap;
	static const std::vector<MemoryMapEntry> MemoryLayout;

	struct MemoryView
	{
		uint8_t* buffer;
		MemoryMapEntry details;

		MemoryView(const MemoryMapEntry details, uint8_t* buffer)
			: details(details), buffer(buffer)
		{}

		bool containsAddress(const MemoryAddress &logicalAddress) const
		{
			return (logicalAddress >= details.logicalBase && logicalAddress < details.logicalEnd);
		}

		void* getPointerToMemory(const MemoryAddress &logicalAddress, size_t &trailingSize) const
		{
			if (!this->containsAddress(logicalAddress))
				return nullptr;

			size_t offset = ((size_t)logicalAddress - (size_t)details.logicalBase);
			trailingSize = details.size - offset;
			return &this->buffer[offset];
		}
	};

	std::vector<MemoryView> views;

	void detach();

	// these helper functions will be implemented for each OS
	static void* obtainSHMHandle();
	static void releaseSHMHandle(const void* handle);
	static uint8_t* obtainView(const void* handle, const MemoryAddress& offset, size_t size);
	static void releaseView(const uint8_t* viewHandle);
};