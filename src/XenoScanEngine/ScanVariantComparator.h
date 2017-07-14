#pragma once
#include "ScannerTypes.h"

#include <stdint.h>


typedef CompareTypeFlags (*ScanVariantComparator)(const void* const source, const void* const check);

template<typename T, size_t N>
struct internalEndianessConverter { };

#define GENERATE_SIZED_ENDIANNESS_CONVERTER(SIZE, STATEMENT) \
	template<typename T> \
	struct internalEndianessConverter<typename T, SIZE> \
	{ \
		constexpr static T swap(T in) \
		{ \
			return STATEMENT; \
		} \
	};

#define GENERATE_TYPE_ALIASED_ENDIANNESS_CONVERTER(TYPE, ALIAS) \
	template<> \
	struct internalEndianessConverter<TYPE, sizeof(TYPE)> \
	{ \
		constexpr static TYPE swap(TYPE in) \
		{ \
			static_assert(sizeof(TYPE) == sizeof(ALIAS), "Sizes of " ## #TYPE ## " and " ## #ALIAS ## " are different"); \
			auto out = internalEndianessConverter<ALIAS, sizeof(ALIAS)>::swap(*reinterpret_cast<ALIAS*>(&in)); \
			return *reinterpret_cast<TYPE*>(&out); \
		} \
	};

GENERATE_SIZED_ENDIANNESS_CONVERTER(1, in)
GENERATE_SIZED_ENDIANNESS_CONVERTER(2, (in << 8) | ((in >> 8) & 0x00ff))
GENERATE_SIZED_ENDIANNESS_CONVERTER(4, (in << 24) | ((in << 8) & 0x00ff0000) | ((in >> 8) & 0x0000ff00) | ((in >> 24) & 0x000000ff))
GENERATE_SIZED_ENDIANNESS_CONVERTER(8, (in = (in & 0x00000000FFFFFFFF) << 32 | (in & 0xFFFFFFFF00000000) >> 32, in = (in & 0x0000FFFF0000FFFF) << 16 | (in & 0xFFFF0000FFFF0000) >> 16, (in & 0x00FF00FF00FF00FF) << 8 | (in & 0xFF00FF00FF00FF00) >> 8))
GENERATE_TYPE_ALIASED_ENDIANNESS_CONVERTER(double, uint64_t)
GENERATE_TYPE_ALIASED_ENDIANNESS_CONVERTER(float, uint32_t)

template<typename T>
constexpr T swapEndianness(const T& in)
{
	return internalEndianessConverter<T, sizeof(T)>::swap(in);
}

template<typename T>
CompareTypeFlags bigEndianNumericComparator(const void* const source, const void* const check)
{
	auto at = *(T*)source;
	auto bt = swapEndianness(*(T*)check);
	if (at == bt) return  Scanner::SCAN_COMPARE_EQUALS;
	else if (at < bt) return  Scanner::SCAN_COMPARE_GREATER_THAN;
	else return Scanner::SCAN_COMPARE_LESS_THAN;
}

template<typename T>
CompareTypeFlags numericComparator(const void* const source, const void* const check)
{
	auto at = *(T*)source;
	auto bt = *(T*)check;
	if (at == bt) return  Scanner::SCAN_COMPARE_EQUALS;
	else if (at < bt) return  Scanner::SCAN_COMPARE_GREATER_THAN;
	else return Scanner::SCAN_COMPARE_LESS_THAN;
}