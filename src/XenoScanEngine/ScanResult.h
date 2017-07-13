#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include "Assert.h"
#include "ScannerTypes.h"
#include "ScannerTarget.h"
#include "FastAllocator.h"


class ScanResultLocation
{
public:
	ScanResultLocation() {}
	~ScanResultLocation() {}

	virtual std::string toString() const = 0;
	virtual bool readCurrentValue(const ScannerTargetShPtr &target, uint8_t* buffer, const size_t &size) const = 0;
	virtual ScanVariant toVariant() const = 0;
};
typedef std::shared_ptr<ScanResultLocation> ScanResultLocationShPtr;

class ScanResultAddress : public ScanResultLocation
{
public:
	ScanResultAddress(MemoryAddress _adr) : adr(_adr)
	{
		char result[100];
		sprintf_s<100>(result, "0x%p", this->adr);
		this->asString = result;
	}
	~ScanResultAddress() {}

	inline virtual std::string toString() const
	{
		return asString;
	}

	inline virtual bool readCurrentValue(const ScannerTargetShPtr &target, uint8_t* buffer, const size_t &size) const
	{
		return target->readArray<uint8_t>(this->adr, size, buffer);
	}

	virtual ScanVariant toVariant() const
	{
		return ScanVariant::FromMemoryAddress(this->adr);
	}

private:
	std::string asString;
	MemoryAddress adr;
};

struct ScanResultMapComparator
{

	//TODO: this needs to be more robust and less slow
	inline bool operator()(const ScanResultLocationShPtr& a, const ScanResultLocationShPtr& b) const
	{
		ASSERT(a.get() && b.get());
		return a->toString() < b->toString();
	}
};

typedef std::allocator<ScanResultAddress> ScanResultAddressAllocator;
typedef std::allocator<ScanVariant> ScanVariantAllocator;
typedef std::vector<ScanVariant, ScanVariantAllocator> ScanResultCollection;
typedef std::pair<ScanResultLocationShPtr, ScanResultCollection> ScanResultMapValueType;

typedef std::allocator<ScanResultMapValueType> ScanResultMapAllocator;
//typedef FastAllocator<ScanResultMapValueType> ScanResultMapAllocator; // custom allocator; currently not enabled

typedef std::map<ScanResultLocationShPtr, ScanResultCollection, ScanResultMapComparator, ScanResultMapAllocator> ScanResultMap;