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

protected:
	virtual bool rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const;
	virtual bool rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const;

private:
	void* sharedMemoryHandle;

	struct MemoryView
	{
		MemoryView(MemoryAddress start, size_t size, uint8_t* buffer)
			: start(start), size(size), buffer(buffer)
		{
			end = (MemoryAddress)((size_t)start + size);
		}
		size_t size;
		MemoryAddress start, end;
		uint8_t* buffer;
	};

	std::vector<MemoryView> views;

	void detach();

	// these helper functions will be implemented for each OS
	static void* obtainSHMHandle();
	static void releaseSHMHandle(const void* handle);
	static uint8_t* obtainView(const void* handle, const MemoryAddress& offset, size_t size);
	static void releaseView(const uint8_t* viewHandle);
};