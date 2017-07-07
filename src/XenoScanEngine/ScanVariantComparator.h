#pragma once
#include "ScannerTypes.h"

#include <stdint.h>


typedef CompareTypeFlags (*ScanVariantComparator)(const void* const source, const void* const check);

template<size_t SIZE>
CompareTypeFlags unsignedNumericComparator(const void* const source, const void* const check)
{
	auto res = memcmp(check, source, SIZE);
	if (res == 0) return Scanner::SCAN_COMPARE_EQUALS;
	else if (res > 0) return Scanner::SCAN_COMPARE_GREATER_THAN;
	else return Scanner::SCAN_COMPARE_LESS_THAN;
}

template<typename T>
CompareTypeFlags signedNumericComparator(const void* const source, const void* const check)
{
	auto at = *(T*)source;
	auto bt = *(T*)check;
	if (at == bt) return  Scanner::SCAN_COMPARE_EQUALS;
	else if (at > bt) return  Scanner::SCAN_COMPARE_GREATER_THAN;
	else return Scanner::SCAN_COMPARE_LESS_THAN;
}