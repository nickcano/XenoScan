#include "LuaEngine.h"

LuaEngine::LuaEngine(void)
{
	for (auto exp = __luaEngineExports.begin(); exp != __luaEngineExports.end(); exp++)
		this->pushGlobal(exp->first, exp->second());
}

LuaEngine::~LuaEngine(void) {}

LuaVariant LuaEngine::createLuaObject(const std::string& typeName, const void* pointer) const
{
	LuaVariant::LuaVariantKTable target;
	target["objectType"] = LuaVariant(typeName);
	target["objectPointer"] = LuaVariant((LuaVariant::LuaVariantPointer)pointer);
	return target;
}

bool LuaEngine::getLuaObject(const LuaVariant& object, const std::string& typeName, void* &pointer) const
{
	LuaVariant::LuaVariantKTable res;
	if (!object.getAsKTable(res)) return false;

	auto it = res.find("objectType");
	if (it == res.end()) return false;

	std::string type;
	if (!it->second.getAsString(type)) return false;
	if (type != typeName) return false;

	it = res.find("objectPointer");
	if (it == res.end()) return false;

	if (!it->second.getAsPointer(pointer)) return false;
	return true;
}

bool LuaEngine::getScannerPair(const LuaVariant& object, ScannerPairList::const_iterator &iterator) const
{
	void* objectPointer;
	if (!this->getLuaObject(object, "ScannerPair", objectPointer)) return false;

	for (auto isearch = this->scanners.cbegin(); isearch != this->scanners.cend(); isearch++)
	{
		if ((void*)(*isearch).get() == objectPointer)
		{
			iterator = isearch;
			return true;
		}
	}
	return false;
}

LuaEngine::ScannerPairShPtr LuaEngine::getArgAsScannerObject(const std::vector<LuaVariant>& args) const
{
	LuaVariant::LuaVariantKTable _scanner;
	args[0].getAsKTable(_scanner);
	ScannerPairList::const_iterator scanner;
	if (!this->getScannerPair(_scanner, scanner)) return nullptr;
	return *scanner;
}

bool LuaEngine::getScanVariantFromLuaVariant(const LuaVariant &variant, const ScanVariant::ScanVariantType &type, bool allowBlank, ScanVariant &output) const
{

	switch (variant.getType())
	{
	case LUA_VARIANT_STRING:
		{
			LuaVariant::LuaVariantString memberValue;
			variant.getAsString(memberValue);
			if (memberValue.length() == 0)
				return false;

			output = ScanVariant::FromStringTyped(memberValue, type);
			return true;
		}
	case LUA_VARIANT_INT:
		{
			if (type < ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN ||
				type > ScanVariant::SCAN_VARIANT_NUMERICTYPES_END)
				return false;

			LuaVariant::LuaVariantInt memberValueInt;
			variant.getAsInt(memberValueInt);
			output = ScanVariant::FromNumberTyped(memberValueInt, type);
			return true;
		}
	case LUA_VARIANT_KTABLE:
		{
			LuaVariant::LuaVariantKTable value;
			if (!variant.getAsKTable(value))
				return false;

			auto itMin = value.find("__min");
			auto itMax = value.find("__max");
			if (itMin != value.end() && itMax != value.end())
			{
				if (type < ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN ||
					type > ScanVariant::SCAN_VARIANT_NUMERICTYPES_END)
					return false;

				LuaVariant::LuaVariantInt minValue, maxValue;
				if (!itMin->second.getAsInt(minValue)) return false;
				if (!itMax->second.getAsInt(maxValue)) return false;

				auto min = ScanVariant::FromNumberTyped(minValue, type);
				auto max = ScanVariant::FromNumberTyped(maxValue, type);
				output = ScanVariant::FromVariantRange(min, max);
				return true;
			}

			if (value.size() == 0 && allowBlank)
			{
				if (type < ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN ||
					type > ScanVariant::SCAN_VARIANT_NUMERICTYPES_END)
					return false;
				output = ScanVariant::MakePlaceholder(type);
				return true;
			}

			return false;
		}
	case LUA_VARIANT_ITABLE:
		{
			LuaVariant::LuaVariantITable value;
			if (!variant.getAsITable(value))
				return false;

			if (value.size() == 0 && allowBlank)
			{
				if (type < ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN ||
					type > ScanVariant::SCAN_VARIANT_NUMERICTYPES_END)
					return false;
				output = ScanVariant::MakePlaceholder(type);
				return true;
			}

			return false;
		}
	default:
		return false;
	}
}

LuaVariant LuaEngine::getLuaVariantFromScanVariant(const ScanVariant &variant) const
{
	// this isn't exactly clean, but we do it to keep the code
	// as fast as possible without too much duplication pre-compilation
#define TYPE_TO_LUA_VARIANT(VAR_TYPE, RAW_TYPE) \
	case ScanVariant::VAR_TYPE: \
	{ \
		RAW_TYPE value; \
		if (variant.getValue(value)) \
			return LuaVariant(value); \
		break; \
	}


	auto traits = variant.getTypeTraits();
	if (variant.isComposite())
	{
		auto values = variant.getCompositeValues();
		LuaVariant::LuaVariantITable complex;
		for (auto v = values.begin(); v != values.end(); v++)
			complex.push_back(this->getLuaVariantFromScanVariant(*v));
		return complex;
	}
	else if (traits->isStringType())
	{
		return LuaVariant(variant.toString());
	}
	else if (traits->isNumericType())
	{
		switch (variant.getType())
		{
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_DOUBLE, double);
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_FLOAT, float);

			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_INT8, int8_t);
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_UINT8, uint8_t);

			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_INT16, int16_t);
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_UINT16, uint16_t);

			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_INT32, int32_t);
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_UINT32, uint32_t);

			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_INT64, int64_t);
			TYPE_TO_LUA_VARIANT(SCAN_VARIANT_UINT64, uint64_t);
		}
	}

	return LuaVariant();

#undef TYPE_TO_LUA_VARIANT
}