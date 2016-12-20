#include "LuaEngine.h"

LuaEngine::LuaEngine(void)
{
	for (auto exp = __luaEngineExports.begin(); exp != __luaEngineExports.end(); exp++)
		this->pushGlobal(exp->first, exp->second);
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

bool LuaEngine::getScanVariantFromLuaVariant(const LuaVariant &variant, const ScanVariant::ScanVariantType &type, ScanVariant &output) const
{

	switch (variant.getType())
	{
	case LUA_VARIANT_STRING:
		{
			LuaVariant::LuaVariantString memberValue;
			variant.getAsString(memberValue);
			if (memberValue.length() == 0)
				return false;

			output = ScanVariant::fromString(memberValue, type);
			return true;
		}
	case LUA_VARIANT_INT:
		{
			if (type < ScanVariant::SCAN_VARIANT_NUMERICTYPES_BEGIN ||
				type > ScanVariant::SCAN_VARIANT_NUMERICTYPES_END)
				return false;

			LuaVariant::LuaVariantInt memberValueInt;
			variant.getAsInt(memberValueInt);
			output = ScanVariant(memberValueInt, type);
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

				auto min = ScanVariant(minValue, type);
				auto max = ScanVariant(maxValue, type);
				output = ScanVariant(min, max);
				return true;
			}
			return false;
		}
	default:
		return false;
	}
}