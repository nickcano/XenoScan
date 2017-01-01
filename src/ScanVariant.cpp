#include "ScanVariant.h"
#include "Scanner.h"

/*
	This cannot be in ScanVariantTypeTraits.h because it needs to know the details
	of ScanVariant. This cannot be in it's own .cpp file because templated member function
	declarations are only visible in the .cpp file in which they are declared.
*/
template <typename TYPE, bool UNSIGNED>
void ScanVariantUnderlyingNumericTypeTraits<TYPE, UNSIGNED>::fromString(const std::wstring& input, ScanVariant& output) const
{
	TYPE value;
	uint8_t buffer[sizeof(int64_t)];
	if (swscanf_s(input.c_str(), this->typeFormat.c_str(), &buffer[0]) == -1)
	{
		output = ScanVariant();
		return;
	}
	memcpy(&value, &buffer[0], sizeof(value));
	output = ScanVariant();
}

ScanVariantUnderlyingTypeTraits* ScanVariant::UnderlyingTypeTraits[ScanVariant::SCAN_VARIANT_NULL + 1] =
{
	new ScanVariantUnderlyingAsciiStringTypeTraits(),
	new ScanVariantUnderlyingWideStringTypeTraits(),

	new ScanVariantUnderlyingNumericTypeTraits<uint8_t, true>(L"uint8", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int8_t, false>(L"int8", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint16_t, true>(L"uint16", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int16_t, false>(L"int16", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint32_t, true>(L"uint32", L"%u"),
	new ScanVariantUnderlyingNumericTypeTraits<int32_t, false>(L"int32", L"%d"),

	new ScanVariantUnderlyingNumericTypeTraits<uint64_t, true>(L"uint64", L"%llu"),
	new ScanVariantUnderlyingNumericTypeTraits<int64_t, false>(L"int64", L"%lld"),

	new ScanVariantUnderlyingNumericTypeTraits<double, true>(L"double", L"%f"),
	new ScanVariantUnderlyingNumericTypeTraits<float, true>(L"float", L"%f"),

	new ScanVariantUnderlyingStructureTypeTraits(),

	new ScanVariantUnderlyingNullTypeTraits()
};

ScanVariant ScanVariant::MakePlaceholder(ScanVariantType type)
{
	ASSERT(type >= SCAN_VARIANT_NUMERICTYPES_BEGIN && type <= SCAN_VARIANT_NUMERICTYPES_END);

	auto offset = type - SCAN_VARIANT_NUMERICTYPES_BEGIN;

	ScanVariant temp;
	temp.type = SCAN_VARIANT_PLACEHOLDER_BEGIN + offset;
	temp.setSizeAndValue();
	return temp;
}

ScanVariant::ScanVariant(const uint8_t* memory, const ScanVariant &reference)
{
	if (reference.isRange())
	{
		this->type = reference.valueStruct[0].getType();
		ASSERT(
			this->type >= SCAN_VARIANT_ALLTYPES_BEGIN &&
			this->type <= SCAN_VARIANT_ALLTYPES_END
		);
	}
	else if (reference.isPlaceholder())
	{
		this->type = reference.type - SCAN_VARIANT_PLACEHOLDER_BEGIN;
		ASSERT(
			this->type >= SCAN_VARIANT_ALLTYPES_BEGIN &&
			this->type <= SCAN_VARIANT_ALLTYPES_END
		);
	}
	else
	{
		this->type = reference.getType();
		ASSERT((
				this->type >= SCAN_VARIANT_ALLTYPES_BEGIN &&
				this->type <= SCAN_VARIANT_ALLTYPES_END
			) ||
			this->isStructure()
		);
	}

	if (this->type == ScanVariant::SCAN_VARIANT_STRUCTURE)
	{
		size_t offset = 0;
		for (auto member = reference.valueStruct.begin(); member != reference.valueStruct.end(); member++)
		{
			ScanVariant temp(&memory[offset], *member);
			this->valueStruct.push_back(temp);
			offset += member->getSize();
		}
		return;
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_ASCII_STRING)
	{
		ASSERT(false); // TODO: implement
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_WIDE_STRING)
	{
		ASSERT(false); // TODO: implement
	}
	else
	{
		auto size = this->getTypeTraits()->getSize();
		memcpy(&this->numericValue, &memory[0], size);
	}

	this->setSizeAndValue();
}

ScanVariant::ScanVariant(const ptrdiff_t &value, const ScanVariantType &type)
{
	this->type = type;
	ASSERT(this->type >= SCAN_VARIANT_NUMERICTYPES_BEGIN && this->type <= SCAN_VARIANT_NUMERICTYPES_END);
	
	auto size = this->getTypeTraits()->getSize();
	ASSERT(sizeof(value) >= size);

	memcpy(&this->numericValue, &value, size);
	this->setSizeAndValue();
}

ScanVariant::ScanVariant(const ScanVariant& min, const ScanVariant& max)
{
	ASSERT(min.getType() == max.getType());
	ASSERT(min.getType() >= SCAN_VARIANT_NUMERICTYPES_BEGIN && min.getType() <= SCAN_VARIANT_NUMERICTYPES_END);

	auto offset = min.getType() - SCAN_VARIANT_NUMERICTYPES_BEGIN;
	this->type = SCAN_VARIANT_RANGE_BEGIN + offset;
	valueStruct.push_back(min);
	valueStruct.push_back(max);

	this->setSizeAndValue();
}

ScanVariant::ScanVariant(const MemoryAddress& valueMemoryAddress)
{
	if (sizeof(MemoryAddress) == sizeof(uint32_t))
	{
		this->valueuint32 = reinterpret_cast<uint32_t>(valueMemoryAddress);
		this->type = SCAN_VARIANT_UINT32;
		this->setSizeAndValue();
	}
	else if (sizeof(MemoryAddress) == sizeof(uint64_t))
	{
		this->valueuint64 = reinterpret_cast<uint64_t>(valueMemoryAddress);
		this->type = SCAN_VARIANT_UINT64;
		this->setSizeAndValue();
	}
	else
	{
		// what went wrong? did we change the MemoryAddress type?
		ASSERT(false);
	}
}

const std::wstring ScanVariant::getTypeName() const
{
	return this->getTypeTraits()->getName();
}

const bool ScanVariant::hasComplexRepresentation() const
{
	return this->getTypeTraits()->isStructureType();
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

const std::vector<std::wstring> ScanVariant::toComplexString() const
{
	std::vector<std::wstring> ret;
	for (auto member = this->valueStruct.begin(); member != this->valueStruct.end(); member++)
		ret.push_back(member->toString());
	return ret;
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

void ScanVariant::compareTo(const uint8_t* memory, CompareTypeFlags &compType) const
{
#define COMPARISON_SET_VALUES1(res) \
	if (res == 0) { compType |= Scanner::SCAN_COMPARE_EQUALS; } \
	else if (res > 0) { compType |= Scanner::SCAN_COMPARE_GREATER_THAN; }\
	else { compType |= Scanner::SCAN_COMPARE_LESS_THAN; }
#define COMPARISON_SET_VALUES2(a, b) \
	if (a == b) { compType |= Scanner::SCAN_COMPARE_EQUALS; } \
	else if (a > b) { compType |= Scanner::SCAN_COMPARE_GREATER_THAN; } \
	else { compType |= Scanner::SCAN_COMPARE_LESS_THAN;} 

	if (this->type == ScanVariant::SCAN_VARIANT_ASCII_STRING)
	{
		std::string::value_type terminator;
		memcpy(&terminator, &memory[this->valueSize - sizeof(terminator)], sizeof(terminator));
		if (terminator == (std::string::value_type)0)
		{
			auto res = strcmp(this->valueAsciiString.c_str(), (std::string::value_type*)&memory[0]);
			COMPARISON_SET_VALUES1(res);
			return;
		}
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_WIDE_STRING)
	{
		std::wstring::value_type terminator;
		memcpy(&terminator, &memory[this->valueSize - sizeof(terminator)], sizeof(terminator));
		if (terminator == (std::wstring::value_type)0)
		{
			auto res = wcscmp(this->valueWideString.c_str(), (std::wstring::value_type*)&memory[0]);
			COMPARISON_SET_VALUES1(res);
			return;
		}
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_DOUBLE)
	{
		double bValue;
		memcpy(&bValue, &memory[0], sizeof(bValue));
		COMPARISON_SET_VALUES2(this->valueDouble, bValue);
		return;
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_FLOAT)
	{
		float bValue;
		memcpy(&bValue, &memory[0], sizeof(bValue));
		COMPARISON_SET_VALUES2(this->valueFloat, bValue);
		return;
	}
	else if (this->type == ScanVariant::SCAN_VARIANT_NULL)
	{
		return;
	}
	else if (this->isStructure())
	{
		compType |= Scanner::SCAN_COMPARE_EQUALS;

		size_t offset = 0;
		size_t iterations = this->valueStruct.size();
		for (size_t i = 0; i < iterations; i++)
		{
			CompareTypeFlags flags = 0;
			this->valueStruct[i].compareTo(&memory[offset], flags);
			if (!(flags & Scanner::SCAN_COMPARE_EQUALS)
				&& !(flags & Scanner::SCAN_COMPARE_ALWAYS_MATCH))
			{
				compType = 0;
				break;
			}
			offset += this->valueStruct[i].getSize();
		}
		return;
	}
	else if (this->isRange())
	{
		CompareTypeFlags minFlags = 0;
		this->valueStruct[0].compareTo(memory, minFlags);
		if (minFlags != Scanner::SCAN_COMPARE_EQUALS &&
			minFlags != Scanner::SCAN_COMPARE_GREATER_THAN)
			return;

		CompareTypeFlags maxFlags = 0;
		this->valueStruct[1].compareTo(memory, maxFlags);
		if (maxFlags != Scanner::SCAN_COMPARE_EQUALS &&
			maxFlags != Scanner::SCAN_COMPARE_LESS_THAN)
			return;

		compType |= Scanner::SCAN_COMPARE_EQUALS;
		return;
	}
	else if (this->isPlaceholder())
	{
		compType |= Scanner::SCAN_COMPARE_ALWAYS_MATCH;
		return;
	}
	else
	{
		auto traits = this->getTypeTraits();
		if (traits->isNumericType())
		{
			if (traits->isSignedNumericType())
			{
				// TODO: this is wrong, signed comparison needs to be more robust
				int8_t data[8];
				memcpy(&data[0], memory, this->valueSize);

				auto thatRightByte = data[this->valueSize - 1];
				bool thatNegative = thatRightByte < 0;

				auto thisRightBye = ((int8_t*)&this->numericValue)[this->valueSize - 1];
				bool thisNegative = thisRightBye < 0;
				int res = 0;
				if (thatNegative == thisNegative)
				{
					if (thatNegative)
						res = memcmp(&this->numericValue, memory, this->valueSize);
					else
						res = memcmp(memory, &this->numericValue, this->valueSize);
				}
				else if (thatNegative)
					res = -1;
				else
					res = 1;
				COMPARISON_SET_VALUES1(res);
			}
			else if (traits->isUnsignedNumericType())
			{
				auto res = memcmp(memory, &this->numericValue, this->valueSize);
				COMPARISON_SET_VALUES1(res);
			}
		}
		return;
	}

#undef COMPARISON_SET_VALUES1
#undef COMPARISON_SET_VALUES2
}

void ScanVariant::searchForMatchesInChunk(
		const uint8_t* chunk,
		const size_t &chunkSize,
		const CompareTypeFlags &compType,
		const MemoryAddress &startAddress,
		std::vector<size_t> &locations) const
{
	ASSERT(this->valueSize > 0);
	if (chunkSize < this->valueSize) return;

	size_t desiredAlignment = this->getTypeTraits()->getAlignment();
	size_t chunkAlignment = (size_t)startAddress % desiredAlignment;
	size_t startOffset = (chunkAlignment == 0) ? 0 : desiredAlignment - chunkAlignment;
	size_t scanEndAt = chunkSize - this->valueSize;

	for (size_t i = startOffset; i <= scanEndAt; )
	{
		CompareTypeFlags res = 0;
		this->compareTo(&chunk[i], res); 
		if ((res & compType) != 0)
		{
			locations.push_back(i);
			// TODO: maybe make overlap checking optional?
			i += this->valueSize;
		}
		else
			i += desiredAlignment;
	}
}


void ScanVariant::setSizeAndValue()
{
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
}


ScanVariant ScanVariant::fromString(const std::string &input, const ScanVariantType type)
{
	std::wstring wideString(input.begin(), input.end());
	return ScanVariant::fromString(wideString, type);
}

ScanVariant ScanVariant::fromString(const std::wstring &input, const ScanVariantType type)
{
	ASSERT(type >= SCAN_VARIANT_ALLTYPES_BEGIN && type <= SCAN_VARIANT_ALLTYPES_END);

	auto traits = UnderlyingTypeTraits[type];

	ScanVariant ret;
	traits->fromString(input, ret);
	return ret;
}