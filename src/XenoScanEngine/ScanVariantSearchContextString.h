#pragma once
#include <stdint.h>
#include "ScannerTypes.h"
#include "ScanVariantTypeTraits.h"
#include "ScanVariant.h"
#include "ScanVariantSearchContextDefault.h"

#include <algorithm>
#include <functional>

/* TODO
	I really need to fix the copy/move issue that's going on with
	ScanVariant, because it's causing a shitstorm with data being
	passed into this class and carrying between copies...
*/
template<typename STRING_TYPE>
class ScanVariantSearchContextString : public ScanVariantSearchContextDefault
{
public:
	ScanVariantSearchContextString(const InternalComparator comp, const STRING_TYPE& str)
		: ScanVariantSearchContextDefault(comp), string(str), searcher(string.cbegin(), string.cend())
	{}

	virtual void searchForMatchesInChunk(
		const ScanVariant* const obj,
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		const bool &isLittleEndian,
		std::vector<size_t> &locations) const
	{
		if (compType == Scanner::SCAN_COMPARE_EQUALS)
		{
			auto schunk = STRING_TYPE(
				(STRING_TYPE::value_type*)&chunk[0],
				(STRING_TYPE::value_type*)&chunk[chunkSize]
			);
			auto res = schunk.cbegin();
			while (true)
			{
				res = std::search(res, schunk.cend(), searcher);
				if (res == schunk.cend())
					break;

				auto index = (res - schunk.cbegin()) * sizeof(STRING_TYPE::value_type);
				locations.push_back(index);
				res++;
			}
		}
		else
		{
			ScanVariantSearchContextDefault::searchForMatchesInChunk(
				obj,
				chunk,
				chunkSize,
				compType,
				startAddress,
				isLittleEndian,
				locations
			);
		}
	}

private:
	typedef std::boyer_moore_horspool_searcher<typename STRING_TYPE::const_iterator> InternalSearcher;
	STRING_TYPE string;
	InternalSearcher searcher;
};
