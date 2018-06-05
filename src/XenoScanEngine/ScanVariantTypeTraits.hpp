#pragma once

/*
	This cannot be in ScanVariantTypeTraits.h because it needs to know the details
	of ScanVariant. This cannot be in it's own .cpp file because templated member function
	declarations are only visible in the .cpp file in which they are declared.

	so we place it in this .hpp and include it where needed
*/
template <typename TYPE, bool UNSIGNED, bool FLOATING, uint32_t BASE_TYPE, uint32_t TARGET_TYPE>
void ScanVariantUnderlyingNumericTypeTraits<TYPE, UNSIGNED, FLOATING, BASE_TYPE, TARGET_TYPE>::fromString(const std::wstring& input, ScanVariant& output) const
{
	TYPE value;
	uint8_t buffer[sizeof(int64_t)];
	if (swscanf_s(input.c_str(), this->typeFormat.c_str(), &buffer[0]) == -1)
	{
		output = ScanVariant::MakeNull();
		return;
	}
	memcpy(&value, &buffer[0], sizeof(value));
	output = ScanVariant::FromNumber(value);
}

template <uint32_t BASE_TYPE, uint32_t TARGET_TYPE>
void ScanVariantUnderlyingAsciiStringTypeTraits<BASE_TYPE, TARGET_TYPE>::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant::FromString(std::string(input.begin(), input.end()));
}

template <uint32_t BASE_TYPE, uint32_t TARGET_TYPE>
void ScanVariantUnderlyingWideStringTypeTraits<BASE_TYPE, TARGET_TYPE>::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant::FromString(input);
}

template <uint32_t BASE_TYPE, uint32_t TARGET_TYPE>
void ScanVariantUnderlyingStructureTypeTraits<BASE_TYPE, TARGET_TYPE>::fromString(const std::wstring& input, ScanVariant& output) const
{
	ASSERT(false); // not possible here
}

template <uint32_t BASE_TYPE, uint32_t TARGET_TYPE>
void ScanVariantUnderlyingNullTypeTraits<BASE_TYPE, TARGET_TYPE>::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant::MakeNull();
}