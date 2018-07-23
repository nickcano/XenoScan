#pragma once
#include <stdint.h>
#include "ScannerTypes.h"
#include "ScanVariantTypeTraits.h"
#include "ScanVariant.h"
#include "ScanVariantSearchContext.h"

class ScanVariantSearchContextDefault : public ScanVariantSearchContext
{
public:
	typedef const CompareTypeFlags(*InternalComparator)(
		const ScanVariant* const obj,
		const ScanVariantComparator &comparator,
		const size_t &valueSize,
		const bool &isLittleEndian,
		const void* const target);

	ScanVariantSearchContextDefault(const InternalComparator comp)
		: internalCompareToBuffer(comp)
	{}

	virtual CompareTypeFlags compareToBuffer(
		const ScanVariant* const obj,
		const bool &isLittleEndian,
		const void* const target) const
	{
		auto size = obj->getSize();
		auto traits = obj->getTypeTraits();
		auto comp = isLittleEndian ? traits->getComparator() : traits->getBigEndianComparator();
		return this->internalCompareToBuffer(obj, comp, size,
			isLittleEndian,
			target
		);
	}

	virtual void searchForMatchesInChunk(
		const ScanVariant* const obj,
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		const bool &isLittleEndian,
		std::vector<size_t> &locations) const
	{
		auto size = obj->getSize();
		auto traits = obj->getTypeTraits();

		size_t desiredAlignment = traits->getAlignment();
		size_t chunkAlignment = (size_t)startAddress % desiredAlignment;
		size_t startOffset = (chunkAlignment == 0) ? 0 : desiredAlignment - chunkAlignment;
		size_t scanEndAt = chunkSize - size;

		auto comp = isLittleEndian ? traits->getComparator() : traits->getBigEndianComparator();

		for (size_t i = startOffset; i <= scanEndAt; )
		{
			auto res = this->internalCompareToBuffer(
				obj, comp, size,
				isLittleEndian,
				&chunk[i]
			);

			// this won't let us use placeholders unless we're in a structure.
			// to change this needs to check for SCAN_COMPARE_ALWAYS_MATCH
			if ((res & compType) != 0)
			{
				locations.push_back(i);
				i += size; // TODO: maybe make overlap checking optional?
			}
			else
				i += desiredAlignment;
		}
	}

private:
	InternalComparator internalCompareToBuffer;
};