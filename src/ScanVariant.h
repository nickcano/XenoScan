#pragma once
#include <string>
#include <vector>

#include "Assert.h"
#include "ScanVariantTypeTraits.h"
#include "ScannerTypes.h"

class ScanVariant
{
public:
	typedef uint32_t ScanVariantType;
	enum _ScanVariantType : ScanVariantType
	{
		// REMEMBER : IMPORTANT
		// When we update this enum, we must also
		// update the ScanVariant::UnderlyingTypeTraits
		// array in ScanVariant.cpp

		SCAN_VARIANT_ALLTYPES_BEGIN,

		SCAN_VARIANT_STRINGTYPES_BEGIN = SCAN_VARIANT_ALLTYPES_BEGIN,
			SCAN_VARIANT_ASCII_STRING = SCAN_VARIANT_STRINGTYPES_BEGIN,
			SCAN_VARIANT_WIDE_STRING,
		SCAN_VARIANT_STRINGTYPES_END = SCAN_VARIANT_WIDE_STRING,

		SCAN_VARIANT_NUMERICTYPES_BEGIN,
			SCAN_VARIANT_UINT8 = SCAN_VARIANT_NUMERICTYPES_BEGIN,
			SCAN_VARIANT_INT8,
			SCAN_VARIANT_UINT16,
			SCAN_VARIANT_INT16,
			SCAN_VARIANT_UINT32,
			SCAN_VARIANT_INT32,
			SCAN_VARIANT_UINT64,
			SCAN_VARIANT_INT64,
			SCAN_VARIANT_DOUBLE,
			SCAN_VARIANT_FLOAT,
		SCAN_VARIANT_NUMERICTYPES_END = SCAN_VARIANT_FLOAT,

		SCAN_VARIANT_ALLTYPES_END = SCAN_VARIANT_NUMERICTYPES_END,

		// These come beyond the end marker because they are special snowflakes
		SCAN_VARIANT_STRUCTURE,
		SCAN_VARIANT_NULL, // null is the last type with traits defined

		// Need to make sure we always handle these types special (check getTypeTraits() function)
		SCAN_VARIANT_RANGE_BEGIN,
		SCAN_VARIANT_RANGE_END = (SCAN_VARIANT_RANGE_BEGIN + (SCAN_VARIANT_NUMERICTYPES_END - SCAN_VARIANT_NUMERICTYPES_BEGIN)),

		SCAN_VARIANT_PLACEHOLDER_BEGIN,
		SCAN_VARIANT_PLACEHOLDER_END = (SCAN_VARIANT_PLACEHOLDER_BEGIN + (SCAN_VARIANT_NUMERICTYPES_END - SCAN_VARIANT_NUMERICTYPES_BEGIN)),
	};

	static ScanVariant MakePlaceholder(ScanVariantType type);

	ScanVariant(const uint8_t* memory, const ScanVariant &reference);
	ScanVariant(const ptrdiff_t &value, const ScanVariantType &type);
	ScanVariant(const ScanVariant& min, const ScanVariant& max);
	ScanVariant(const MemoryAddress& valueMemoryAddress);
	ScanVariant(std::string valueAsciiString)                   : valueAsciiString(valueAsciiString),     type(SCAN_VARIANT_ASCII_STRING) { setSizeAndValue(); }
	ScanVariant(std::wstring valueWideString)                   : valueWideString(valueWideString),       type(SCAN_VARIANT_WIDE_STRING) { setSizeAndValue(); }
	ScanVariant(uint8_t valueuint8)                             : valueuint8(valueuint8),                 type(SCAN_VARIANT_UINT8) { setSizeAndValue(); }
	ScanVariant(int8_t valueint8)                               : valueint8(valueint8),                   type(SCAN_VARIANT_INT8) { setSizeAndValue(); }
	ScanVariant(uint16_t valueuint16)                           : valueuint16(valueuint16),               type(SCAN_VARIANT_UINT16) { setSizeAndValue(); }
	ScanVariant(int16_t valueint16)                             : valueint16(valueint16),                 type(SCAN_VARIANT_INT16) { setSizeAndValue(); }
	ScanVariant(uint32_t valueuint32)                           : valueuint32(valueuint32),               type(SCAN_VARIANT_UINT32) { setSizeAndValue(); }
	ScanVariant(int32_t valueint32)                             : valueint32(valueint32),                 type(SCAN_VARIANT_INT32) { setSizeAndValue(); }
	ScanVariant(uint64_t valueuint64)                           : valueuint64(valueuint64),               type(SCAN_VARIANT_UINT64) { setSizeAndValue(); }
	ScanVariant(int64_t valueint64)                             : valueint64(valueint64),                 type(SCAN_VARIANT_INT64) { setSizeAndValue(); }
	ScanVariant(double valueDouble)                             : valueDouble(valueDouble),               type(SCAN_VARIANT_DOUBLE) { setSizeAndValue(); }
	ScanVariant(float valueFloat)                               : valueFloat(valueFloat),                 type(SCAN_VARIANT_FLOAT) { setSizeAndValue(); }
	ScanVariant(const std::vector<ScanVariant> &valueStruct)    : valueStruct(valueStruct),               type(SCAN_VARIANT_STRUCTURE) { setSizeAndValue(); }
	ScanVariant()                                               :                                         type(SCAN_VARIANT_NULL) { }


	inline const size_t getSize() const
	{
		return this->valueSize;
	}
	inline const ScanVariantType getType() const
	{
		return this->type;
	}
	inline const ScanVariantUnderlyingTypeTraits* getTypeTraits() const
	{
		if (this->isRange())
		{
			auto offset = this->getType() - SCAN_VARIANT_RANGE_BEGIN;
			return UnderlyingTypeTraits[SCAN_VARIANT_NUMERICTYPES_BEGIN + offset];
		}
		else if (this->isPlaceholder())
		{
			auto offset = this->getType() - SCAN_VARIANT_PLACEHOLDER_BEGIN;
			return UnderlyingTypeTraits[SCAN_VARIANT_NUMERICTYPES_BEGIN + offset];
		}
		else
			return UnderlyingTypeTraits[this->getType()];
	}

	const std::wstring getTypeName() const;
	const bool hasComplexRepresentation() const;
	const std::wstring toString() const;
	const std::vector<std::wstring> toComplexString() const;

	inline const bool isStructure() const
	{
		return this->getTypeTraits()->isStructureType();
	}
	inline const bool isRange() const
	{
		return (this->type >= SCAN_VARIANT_RANGE_BEGIN && this->type <= SCAN_VARIANT_RANGE_END);
	}

	inline const bool isPlaceholder() const
	{
		return (this->type >= SCAN_VARIANT_PLACEHOLDER_BEGIN && this->type <= SCAN_VARIANT_PLACEHOLDER_END);
	}

	const bool isNull() const;
	const bool getValue(std::string &value) const;
	const bool getValue(std::wstring &value) const;
	const bool getValue(uint8_t &value) const;
	const bool getValue(int8_t &value) const;
	const bool getValue(uint16_t &value) const;
	const bool getValue(int16_t &value) const;
	const bool getValue(uint32_t &value) const;
	const bool getValue(int32_t &value) const;
	const bool getValue(uint64_t &value) const;
	const bool getValue(int64_t &value) const;
	const bool getValue(double &value) const;
	const bool getValue(float &value) const;
	const bool getValue(std::vector<ScanVariant> &value) const;

	/*
		This is safe IF and ONLY IF the caller takes some precautions:
			1. When comparing a ScanVariant to a raw memory buffer, the caller should ensure
				the memory buffer is the same size of, or greater than the size of, the ScanVariant.
			2. This means that we need to be sure there's no race conditions or any of circumstances
				that can lead to the ScanVariant size changing between allocating the buffer, reading
				the memory, and using one of these.
			Realistically, when possible, we should try to use ScanVariant::compareTo
	*/
	// TODO: test string scans, test all integer type scans
	void compareTo(const uint8_t* memory, CompareTypeFlags &compType) const;

	void searchForMatchesInChunk(
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		std::vector<size_t> &locations) const;

	static ScanVariant fromString(const std::string &input, const ScanVariantType type);
	static ScanVariant fromString(const std::wstring &input, const ScanVariantType type);

private:
	static ScanVariantUnderlyingTypeTraits* UnderlyingTypeTraits[SCAN_VARIANT_NULL + 1];

	void ScanVariant::init();

	ScanVariantType type;
	std::string valueAsciiString;
	std::wstring valueWideString;
	std::vector<ScanVariant> valueStruct;
	union
	{
		uint8_t numericValue;

		uint8_t valueuint8;
		int8_t valueint8;
		uint16_t valueuint16;
		int16_t valueint16;
		uint32_t valueuint32;
		int32_t valueint32;
		uint64_t valueuint64;
		int64_t valueint64;
		double valueDouble;
		float valueFloat;
	};

	size_t valueSize;


	void setSizeAndValue();
};