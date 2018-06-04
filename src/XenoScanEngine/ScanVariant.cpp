#include "ScanVariant.h"
#include "Scanner.h"

#include "ScanVariantTypeTraits.hpp"

ScanVariantUnderlyingTypeTraits* ScanVariant::UnderlyingTypeTraits[ScanVariant::SCAN_VARIANT_NULL + 1] =
{
	new ScanVariantUnderlyingAsciiStringTypeTraits<ScanVariant::SCAN_VARIANT_ASCII_STRING, ScanVariant::SCAN_VARIANT_ASCII_STRING>(),
	new ScanVariantUnderlyingWideStringTypeTraits<ScanVariant::SCAN_VARIANT_WIDE_STRING,   ScanVariant::SCAN_VARIANT_WIDE_STRING>(),

	new ScanVariantUnderlyingNumericTypeTraits<uint8_t, true, false,  ScanVariant::SCAN_VARIANT_UINT8,   ScanVariant::SCAN_VARIANT_UINT8>(L"uint8", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int8_t, false, false,  ScanVariant::SCAN_VARIANT_INT8,    ScanVariant::SCAN_VARIANT_INT8>(L"int8", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint16_t, true, false, ScanVariant::SCAN_VARIANT_UINT16,  ScanVariant::SCAN_VARIANT_UINT16>(L"uint16", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int16_t, false, false, ScanVariant::SCAN_VARIANT_INT16,   ScanVariant::SCAN_VARIANT_INT16>(L"int16", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint32_t, true, false, ScanVariant::SCAN_VARIANT_UINT32,  ScanVariant::SCAN_VARIANT_UINT32>(L"uint32", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int32_t, false, false, ScanVariant::SCAN_VARIANT_INT32,   ScanVariant::SCAN_VARIANT_INT32>(L"int32", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint64_t, true, false, ScanVariant::SCAN_VARIANT_UINT64,  ScanVariant::SCAN_VARIANT_UINT64>(L"uint64", L"%llu"),
	new ScanVariantUnderlyingNumericTypeTraits<int64_t, false, false, ScanVariant::SCAN_VARIANT_INT64,   ScanVariant::SCAN_VARIANT_INT64>(L"int64", L"%lld"),

	new ScanVariantUnderlyingNumericTypeTraits<double, true, true,    ScanVariant::SCAN_VARIANT_DOUBLE,  ScanVariant::SCAN_VARIANT_DOUBLE>(L"double", L"%f"),
	new ScanVariantUnderlyingNumericTypeTraits<float, true, true,     ScanVariant::SCAN_VARIANT_FLOAT,   ScanVariant::SCAN_VARIANT_FLOAT>(L"float", L"%f"),

	// these dynamic types are unsigned, but base types are signed to allow for negative offsets
	new ScanVariantUnderlyingNumericTypeTraits<uint64_t, true, false, ScanVariant::SCAN_VARIANT_INT64,   ScanVariant::SCAN_VARIANT_UINT64>(L"filetime64", L"%llu"),
	new ScanVariantUnderlyingNumericTypeTraits<uint32_t, true, false, ScanVariant::SCAN_VARIANT_INT32,   ScanVariant::SCAN_VARIANT_UINT32>(L"ticktime32", L"%u"),

	new ScanVariantUnderlyingStructureTypeTraits<ScanVariant::SCAN_VARIANT_STRUCTURE, ScanVariant::SCAN_VARIANT_STRUCTURE>(),

	new ScanVariantUnderlyingNullTypeTraits<ScanVariant::SCAN_VARIANT_NULL, ScanVariant::SCAN_VARIANT_NULL>()
};

const ScanVariant ScanVariant::MakePlaceholder(const ScanVariantType& type)
{
	ASSERT(type >= SCAN_VARIANT_NUMERICTYPES_BEGIN && type <= SCAN_VARIANT_NUMERICTYPES_END);

	auto offset = type - SCAN_VARIANT_NUMERICTYPES_BEGIN;

	ScanVariant temp;
	temp.type = SCAN_VARIANT_PLACEHOLDER_BEGIN + offset;
	temp.setSizeAndValue();
	return temp;
}

const ScanVariant ScanVariant::FromRawBuffer(const void* buffer, const size_t& bufferSize, const bool &isLittleEndian, const ScanVariant& reference)
{
	return ScanVariant::FromRawBuffer((uint8_t*)buffer, bufferSize, isLittleEndian, reference);
}
const ScanVariant ScanVariant::FromRawBuffer(const uint8_t* buffer, const size_t& bufferSize, const bool &isLittleEndian, const ScanVariant& reference)
{
	if (reference.isRange())
		return ScanVariant::FromRawBuffer(buffer, bufferSize, isLittleEndian, reference.valueStruct[0]);
	else if (reference.isStructure())
	{
		// structure requires access to the reference variant for member and type information
		ScanVariant v;
		v.type = reference.getType();
		size_t offset = 0;
		for (auto member = reference.valueStruct.begin(); member != reference.valueStruct.end(); member++)
		{
			auto vmember = ScanVariant::FromRawBuffer(&buffer[offset], bufferSize - offset, isLittleEndian, *member);
			v.valueStruct.push_back(vmember);
			offset += member->getSize();
		}
		v.setSizeAndValue();
		return v;
	}
	else if (reference.getType() == ScanVariant::SCAN_VARIANT_ASCII_STRING)
	{
		auto sizeInBytes = reference.valueAsciiString.length() * sizeof(std::string::value_type); // todo maybe use the build in value size field
		ASSERT(sizeInBytes <= bufferSize);

		std::string value;
		reference.getTypeTraits()->copyFromBuffer(buffer, sizeInBytes, isLittleEndian, &value);
		return ScanVariant::FromString(value);
	}
	else if (reference.getType() == ScanVariant::SCAN_VARIANT_WIDE_STRING)
	{
		auto sizeInBytes = reference.valueWideString.length() * sizeof(std::wstring::value_type); // same as above
		ASSERT(sizeInBytes <= bufferSize);

		std::wstring value;
		reference.getTypeTraits()->copyFromBuffer(buffer, sizeInBytes, isLittleEndian, &value);
		return ScanVariant::FromString(value);
	}
	else
	{
		ScanVariant v;
		v.type = reference.getType();
		if (reference.isPlaceholder())
			v.type = (reference.getType() - SCAN_VARIANT_PLACEHOLDER_BEGIN) + SCAN_VARIANT_NUMERICTYPES_BEGIN;
		else if (reference.isDynamic())
			v.type = reference.getTypeTraits()->getTargetType();

		auto traits = v.getTypeTraits();
		auto size = traits->getSize();
		ASSERT(size <= bufferSize);

		traits->copyFromBuffer(buffer, size, isLittleEndian, &v.numericValue);
		v.setSizeAndValue();
		return v;
	}

	ASSERT(false);
	return ScanVariant();
}

const ScanVariant ScanVariant::FromVariantRange(const ScanVariant& min, const ScanVariant& max)
{
	ASSERT(min.getType() == max.getType());
	ASSERT(min.getType() >= SCAN_VARIANT_NUMERICTYPES_BEGIN && min.getType() <= SCAN_VARIANT_NUMERICTYPES_END);

	ScanVariant v;
	auto offset = min.getType() - SCAN_VARIANT_NUMERICTYPES_BEGIN;
	v.type = SCAN_VARIANT_RANGE_BEGIN + offset;
	v.valueStruct.push_back(min);
	v.valueStruct.push_back(max);

	v.setSizeAndValue();
	return v;
}

const ScanVariant ScanVariant::FromMemoryAddress(const MemoryAddress& valueMemoryAddress)
{
	ScanVariant v;
	if (sizeof(MemoryAddress) == sizeof(uint32_t))
	{
		v.valueuint32 = reinterpret_cast<uint32_t>(valueMemoryAddress);
		v.type = SCAN_VARIANT_UINT32;
		v.setSizeAndValue();
	}
	else if (sizeof(MemoryAddress) == sizeof(uint64_t))
	{
		v.valueuint64 = reinterpret_cast<uint64_t>(valueMemoryAddress);
		v.type = SCAN_VARIANT_UINT64;
		v.setSizeAndValue();
	}
	else
	{
		// what went wrong? did we change the MemoryAddress type?
		ASSERT(false);
	}
	return v;
}

const ScanVariant ScanVariant::FromNumberTyped(const uint64_t &value, const ScanVariantType &type)
{
	ASSERT(type >= SCAN_VARIANT_NUMERICTYPES_BEGIN && type <= SCAN_VARIANT_NUMERICTYPES_END);

	ScanVariant v;
	v.type = type;
	
	auto traits = v.getTypeTraits();
	if (traits->isDynamicType())
	{
		// dynamic types have an underlying numeric type
		// which acts as a delta to the dynamic value.
		// this delta gets stored as the first element in
		// `valueStruct` so that we can use it while preparing
		// whatever is required by the dynamic type
		ScanVariant b;
		b.type = traits->getBaseType();

		auto size = b.getTypeTraits()->getSize();
		ASSERT(sizeof(value) >= size);
		memcpy(&b.numericValue, &value, size);
		b.setSizeAndValue();
		v.valueStruct.push_back(b);
	}
	else
	{
		auto size = traits->getSize();
		ASSERT(sizeof(value) >= size);
		memcpy(&v.numericValue, &value, size);
	}

	v.setSizeAndValue();
	return v;
}

const ScanVariant ScanVariant::FromStringTyped(const std::string& input, const ScanVariantType& type)
{
	// TODO maybe handle dynamic types here?
	std::wstring wideString(input.begin(), input.end());
	return ScanVariant::FromStringTyped(wideString, type);
}
const ScanVariant ScanVariant::FromStringTyped(const std::wstring& input, const ScanVariantType& type)
{
	ASSERT(type >= SCAN_VARIANT_ALLTYPES_BEGIN && type <= SCAN_VARIANT_ALLTYPES_END);

	// TODO maybe handle dynamic types here?
	auto traits = UnderlyingTypeTraits[type];
	ScanVariant ret;
	traits->fromString(input, ret);
	return ret;
}

const ScanVariant ScanVariant::FromTargetMemory(const std::shared_ptr<class ScannerTarget> &target, const MemoryAddress& address, const ScanVariantType& type)
{
	ASSERT(target.get());
	ASSERT(type >= SCAN_VARIANT_ALLTYPES_BEGIN && type <= SCAN_VARIANT_ALLTYPES_END);

	// TODO: add structure support

	auto traits = UnderlyingTypeTraits[type];
	if (type == ScanVariant::SCAN_VARIANT_ASCII_STRING)
	{
		std::string value;
		if (!target->readString(address, value))
			return ScanVariant::MakeNull();
		return ScanVariant::FromString(value);
	}
	else if (type == ScanVariant::SCAN_VARIANT_WIDE_STRING)
	{
		// I don't think we'll ever need implement endianness flip for strings but,
		// if we do, we'll need to update this and ScanVariantUnderlyingWideStringTypeTraits
		// to support it
		std::wstring value;
		if (!target->readString(address, value))
			return ScanVariant::MakeNull();
		return ScanVariant::FromString(value);
	}
	else if (traits->isNumericType())
	{
		auto targetType = type;
		if (traits->isDynamicType())
			targetType = traits->getTargetType();

		auto size = traits->getSize();
		auto buffer = new uint8_t[size];
		if (!target->readArray(address, size, buffer))
		{
			delete[] buffer;
			return ScanVariant::MakeNull();
		}
		
		ScanVariant v;
		traits->copyFromBuffer(buffer, size, target->isLittleEndian(), &v.numericValue);
		delete[] buffer;

		v.type = targetType;
		v.setSizeAndValue();
		return v;
	}

	return ScanVariant::MakeNull();
}

const bool ScanVariant::isCompatibleWith(const ScanVariant& other, const bool strict) const
{
	auto thisUnderType = this->getUnderlyingType();
	auto thisUnderTraits = this->getTypeTraits();
	auto otherUnderType = other.getUnderlyingType();
	auto otherUnderTraits = other.getTypeTraits();
	
	if (thisUnderType == SCAN_VARIANT_STRUCTURE)
	{
		if (otherUnderType != thisUnderType)
			return false;
		if (this->valueStruct.size() != other.valueStruct.size())
			return false;
		// TODO would be cool to use variadic templates to map a zip iterator for range-based loops of multiple collections...
		for (size_t i = 0; i != this->valueStruct.size(); i++)
			if (!this->valueStruct[i].isCompatibleWith(other.valueStruct[i], false))
				return false;
		return true;
	}
	else if (thisUnderTraits->isNumericType())
	{
		if (strict) // don't allow mix-match, typically because the first scan was of loose types to begin with an rescan will have duplicates at same address but slightly different types
			return (thisUnderTraits->getSize() == otherUnderTraits->getSize()
				&& thisUnderTraits->isSignedNumericType() == otherUnderTraits->isSignedNumericType()
				&& thisUnderTraits->isFloatingPointNumericType() == otherUnderTraits->isFloatingPointNumericType());
		else // allow mix-match of numeric types as long as size is the same
			return (thisUnderTraits->getSize() == otherUnderTraits->getSize());
	}
	return (thisUnderType == otherUnderType);
}


const std::wstring ScanVariant::getTypeName() const
{
	return this->getTypeTraits()->getName();
}

const bool ScanVariant::isComposite() const
{
	return this->getTypeTraits()->isStructureType();
}
const std::vector<ScanVariant>& ScanVariant::getCompositeValues() const
{
	return this->valueStruct;
}

const std::wstring ScanVariant::toString() const
{
	auto traits = this->getTypeTraits();
	if (traits->isNumericType())
		return traits->toString((void*)&numericValue);
	else if (traits->isStringType())
	{
		if (this->getType() == SCAN_VARIANT_ASCII_STRING)
			return traits->toString((void*)this->valueAsciiString.c_str());
		else if (this->getType() == SCAN_VARIANT_WIDE_STRING)
			return traits->toString((void*)this->valueWideString.c_str());
	}

	return traits->toString(nullptr);
}

const bool ScanVariant::isNull() const
{
	return (this->type == SCAN_VARIANT_NULL);
}

const bool ScanVariant::getValue(std::string &value) const
{
	if (this->type == SCAN_VARIANT_ASCII_STRING)
	{
		value = valueAsciiString;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(std::wstring &value) const
{
	if (this->type == SCAN_VARIANT_WIDE_STRING)
	{
		value = valueWideString;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(uint8_t &value) const
{
	if (this->type == SCAN_VARIANT_UINT8)
	{
		value = valueuint8;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(int8_t &value) const
{
	if (this->type == SCAN_VARIANT_INT8)
	{
		value = valueint8;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(uint16_t &value) const
{
	if (this->type == SCAN_VARIANT_UINT16)
	{
		value = valueuint16;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(int16_t &value) const
{
	if (this->type == SCAN_VARIANT_INT16)
	{
		value = valueint16;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(uint32_t &value) const
{
	if (this->type == SCAN_VARIANT_UINT32)
	{
		value = valueuint32;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(int32_t &value) const
{
	if (this->type == SCAN_VARIANT_INT32)
	{
		value = valueint32;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(uint64_t &value) const
{
	if (this->type == SCAN_VARIANT_UINT64)
	{
		value = valueuint64;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(int64_t &value) const
{
	if (this->type == SCAN_VARIANT_INT64)
	{
		value = valueint64;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(double &value) const
{
	if (this->type == SCAN_VARIANT_DOUBLE)
	{
		value = valueDouble;
		return true;
	}
	return false;
}
const bool ScanVariant::getValue(float &value) const
{
	if (this->type == SCAN_VARIANT_FLOAT)
	{
		value = valueFloat;
		return true;
	}
	return false;
}

const bool ScanVariant::getValue(std::vector<ScanVariant> &value) const
{
	if (this->type == SCAN_VARIANT_STRUCTURE)
	{
		value = valueStruct;
		return true;
	}
	return false;
}

const bool ScanVariant::writeToTarget(const std::shared_ptr<class ScannerTarget> &target, const MemoryAddress& address) const
{
	ASSERT(target.get());
	ASSERT(type >= SCAN_VARIANT_ALLTYPES_BEGIN && type <= SCAN_VARIANT_ALLTYPES_END);

	// TODO: add support for structures
	// TODO: figure out what to do about dynamic types here

	auto traits = this->getTypeTraits();
	if (type == ScanVariant::SCAN_VARIANT_ASCII_STRING)
		return target->writeString(address, this->valueAsciiString);
	else if (type == ScanVariant::SCAN_VARIANT_WIDE_STRING)
		return target->writeString(address, this->valueWideString); // doubt it but if we ever endianness swap strings we'll need to fix this and FromTargetMemory
	else if (traits->isNumericType())
	{
		auto size = traits->getSize();
		auto buffer = new uint8_t[size];
		traits->copyFromBuffer(&this->numericValue, size, target->isLittleEndian(), buffer);

		auto ret = target->writeArray(address, size, buffer);

		delete[] buffer;

		return ret;
	}
	return false;
}

void ScanVariant::prepareForSearch(const ScannerTarget* const target)
{
	// TODO: we probably want to re-write the endianess code to set up comparators
	// inside of this function. 

	auto traits = this->getTypeTraits();
	if (traits->isStructureType() || this->isRange())
	{
		for (auto member = this->valueStruct.begin(); member != this->valueStruct.end(); member++)
			member->prepareForSearch(target);
	}
	else if (traits->isDynamicType())
	{
		// valuestruct holds one object: the offset from the base value
		ASSERT(this->valueStruct.size() == 1);
		ASSERT(this->valueStruct[0].type == traits->getBaseType());

		// TODO: can we abstract this more so type stuff is transparent?
		if (this->type == SCAN_VARIANT_FILETIME64)
		{
			auto delta = this->valueStruct[0].valueint64;
			this->valueuint64 = target->getFileTime64();
			if (delta >= 0) // postive, so add
				this->valueuint64 += static_cast<uint64_t>(delta);
			else // subtract but do it without type mismatch
				this->valueuint64 -= static_cast<uint64_t>(delta * -1);
		}
		else if (this->type == SCAN_VARIANT_TICKTIME32)
		{
			auto delta = this->valueStruct[0].valueint32;
			this->valueuint32 = target->getTickTime32();
			if (delta >= 0) // postive, so add
				this->valueuint32 += static_cast<uint32_t>(delta);
			else // subtract but do it without type mismatch
				this->valueuint32 -= static_cast<uint32_t>(delta * -1);
		}
		else
			ASSERT(false);
	}
}

void ScanVariant::searchForMatchesInChunk(
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		const bool &isLittleEndian,
		std::vector<size_t> &locations) const
{
	ASSERT(this->valueSize > 0);
	ASSERT(this->compareToBuffer != nullptr); // can happen if it's a null scan variant
	if (chunkSize < this->valueSize) return;

	auto traits = this->getTypeTraits();
	size_t desiredAlignment = traits->getAlignment();
	size_t chunkAlignment = (size_t)startAddress % desiredAlignment;
	size_t startOffset = (chunkAlignment == 0) ? 0 : desiredAlignment - chunkAlignment;
	size_t scanEndAt = chunkSize - this->valueSize;

	auto comp = isLittleEndian ? traits->getComparator() : traits->getBigEndianComparator();
	auto size = this->valueSize;
	auto numericValue = &this->numericValue;
	auto asciiValue = this->valueAsciiString.c_str();
	auto wideValue = this->valueWideString.c_str();

	for (size_t i = startOffset; i <= scanEndAt; )
	{
		auto res = this->compareToBuffer(
			this, comp, size,
			isLittleEndian,
			numericValue,
			asciiValue, wideValue,
			&chunk[i]
		);

		// this won't let us use placeholders unless we're in a structure.
		// to change this needs to check for SCAN_COMPARE_ALWAYS_MATCH
		if ((res & compType) != 0)
		{
			locations.push_back(i);
			i += this->valueSize; // TODO: maybe make overlap checking optional?
		}
		else
			i += desiredAlignment;
	}
}

const CompareTypeFlags ScanVariant::compareRangeToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	auto minRes = comparator(&obj->valueStruct[0].numericValue, target);
	if (!(minRes & Scanner::SCAN_COMPARE_GREATER_THAN_OR_EQUALS))
		return Scanner::SCAN_COMPARE_LESS_THAN;
	auto maxRes = comparator(&obj->valueStruct[1].numericValue, target);
	if (!(maxRes & Scanner::SCAN_COMPARE_LESS_THAN_OR_EQUALS))
		return Scanner::SCAN_COMPARE_GREATER_THAN;
	return Scanner::SCAN_COMPARE_EQUALS;
}
const CompareTypeFlags ScanVariant::compareNumericToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	return comparator(numericBuffer, target);
}

const CompareTypeFlags ScanVariant::comparePlaceholderToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	return Scanner::SCAN_COMPARE_ALWAYS_MATCH;
}
const CompareTypeFlags ScanVariant::compareStructureToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	size_t offset = 0;
	auto buf = (uint8_t*)target;
	size_t iterations = obj->valueStruct.size();
	for (size_t i = 0; i < iterations; i++)
	{
		auto innerObj = &obj->valueStruct[i];
		auto res = innerObj->compareTo(&buf[offset], isLittleEndian);
		if (!(res & Scanner::SCAN_COMPARE_EQUALS)
			&& !(res & Scanner::SCAN_COMPARE_ALWAYS_MATCH))
		{
			return 0;
		}
		offset += obj->valueStruct[i].getSize();
	}
	return Scanner::SCAN_COMPARE_EQUALS;
}
const CompareTypeFlags ScanVariant::compareAsciiStringToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	auto buf = (uint8_t*)target;
	std::string::value_type terminator;
	memcpy(&terminator, &buf[valueSize - sizeof(terminator)], sizeof(terminator));
	if (terminator == (std::string::value_type)0)
	{
		auto res = strcmp((std::string::value_type*)asciiBuffer, (std::string::value_type*)target);
		if (res == 0) return Scanner::SCAN_COMPARE_EQUALS;
		else if (res > 0) return Scanner::SCAN_COMPARE_GREATER_THAN;
		else return Scanner::SCAN_COMPARE_LESS_THAN;
	}
	return 0;
}
const CompareTypeFlags ScanVariant::compareWideStringToBuffer(
	const ScanVariant* const obj,
	const ScanVariantComparator &comparator,
	const size_t &valueSize,
	const bool &isLittleEndian,
	const void* const numericBuffer,
	const void* const asciiBuffer,
	const void* const wideBuffer,
	const void* const target)
{
	auto buf = (uint8_t*)target;
	std::wstring::value_type terminator;
	memcpy(&terminator, &buf[valueSize - sizeof(terminator)], sizeof(terminator));
	if (terminator == (std::wstring::value_type)0)
	{
		auto res = wcscmp((std::wstring::value_type*)wideBuffer, (std::wstring::value_type*)target);
		if (res == 0) return Scanner::SCAN_COMPARE_EQUALS;
		else if (res > 0) return Scanner::SCAN_COMPARE_GREATER_THAN;
		else return Scanner::SCAN_COMPARE_LESS_THAN;
	}
	return 0;
}

void ScanVariant::setSizeAndValue()
{
	this->compareToBuffer = nullptr;

	// first, we'll set the size and value properly
	auto traits = this->getTypeTraits();
	if (traits->isNumericType())
		this->valueSize = traits->getSize();
	else if (traits->isStringType())
	{
		if (this->getType() == SCAN_VARIANT_ASCII_STRING)
			this->valueSize = this->valueAsciiString.length() + sizeof(std::string::value_type);
		else if (this->getType() == SCAN_VARIANT_WIDE_STRING)
			this->valueSize = this->valueWideString.length() * sizeof(std::wstring::value_type) + sizeof(std::wstring::value_type);
	}
	else if (traits->isStructureType())
	{
		this->valueSize = 0;
		for (auto member = this->valueStruct.begin(); member != this->valueStruct.end(); member++)
			this->valueSize += member->getSize();
	}

	// next, we'll set up the proper comparator
	// we do this to avoid type-checking at compare time.
	// we avoid std:bind and other helps for speed.
	if (this->isRange())
		this->compareToBuffer = &ScanVariant::compareRangeToBuffer;
	else if (this->isPlaceholder())
		this->compareToBuffer = &ScanVariant::comparePlaceholderToBuffer;
	else if (traits->isNumericType())
		this->compareToBuffer = &ScanVariant::compareNumericToBuffer;
	else if (this->getType() == SCAN_VARIANT_ASCII_STRING)
		this->compareToBuffer = &ScanVariant::compareAsciiStringToBuffer;
	else if (this->getType() == SCAN_VARIANT_WIDE_STRING)
		this->compareToBuffer = &ScanVariant::compareWideStringToBuffer;
	else if (traits->isStructureType())
		this->compareToBuffer = &ScanVariant::compareStructureToBuffer;
	else if (this->getType() != SCAN_VARIANT_NULL)
		ASSERT(false); // didn't find a comparator!
}