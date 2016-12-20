#include "ScanVariantTypeTraits.h"
#include "ScanVariant.h"

void ScanVariantUnderlyingAsciiStringTypeTraits::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant(std::string(input.begin(), input.end()));
}

void ScanVariantUnderlyingWideStringTypeTraits::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant(input);
}

void ScanVariantUnderlyingStructureTypeTraits::fromString(const std::wstring& input, ScanVariant& output) const
{
	ASSERT(false); // not possible here
}

void ScanVariantUnderlyingNullTypeTraits::fromString(const std::wstring& input, ScanVariant& output) const
{
	output = ScanVariant();
}