#pragma once
#include <stdint.h>
#include "ScannerTypes.h"
#include "ScanVariantTypeTraits.h"

class ScanVariantSearchContext
{
public:
	virtual CompareTypeFlags compareToBuffer(
		const ScanVariant* const obj,
		const bool &isLittleEndian,
		const void* const target) const = 0;

	virtual void searchForMatchesInChunk(
		const ScanVariant* const obj,
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		const bool &isLittleEndian,
		std::vector<size_t> &locations) const = 0;
};