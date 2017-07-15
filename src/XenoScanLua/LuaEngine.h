#pragma once
#include <vector>
#include <list>
#include <iostream>

#include "XenoLua/LuaPrimitive.h"

#include "XenoScanEngine/ScannerTarget.h"
#include "XenoScanEngine/Scanner.h"
#include "XenoScanEngine/KeyedFactory.h"


#define LUAENGINE_EXPORT_VALUE(type, name, val) __LuaEngineExporter __exporter ## name (#name, (type)val);
#define LUAENGINE_EXPORT_FUNCTION(uniq, name) \
	int __exporter__func ## uniq(lua_State *L) { \
		auto inst = LuaPrimitive::getInstance<LuaEngine>(L); \
		if (inst != nullptr) return inst-> ## uniq (); \
		return 0; \
	} \
	__LuaEngineExporter __exporter ## uniq (name, LuaVariant(& __exporter__func ## uniq));

#define LUAENGINE_EXPORT_FACTORY_KEYS(factoryType, factory, name) \
	__LuaEngineFactoryKeyExporter<factoryType::KEY_TYPE, factoryType::BASE_TYPE>  __factoryKeyExporter ## name (#name, factory)

extern std::vector<std::pair<const std::string, const std::function<const LuaVariant()>>> __luaEngineExports;

class LuaEngine : public LuaPrimitive
{
public:
	LuaEngine();
	~LuaEngine();

	/* EXPORTED FUNCTIONS */
	int attach();
	int destroy();

	int readMemory();
	int writeMemory();

	int newScan();
	int runScan();
	int getScanResultsSize();
	int getScanResults();
	int getDataStructures();

protected:
	virtual void displayError(std::string error, bool fatal)
	{
		std::cout << (fatal ? "[FTL] " : "[ERR] ");
		std::cout << error << std::endl;
	}

private:
	struct ScannerPair
	{
		ScannerTargetShPtr target;
		ScannerShPtr scanner;
	};
	typedef std::shared_ptr<ScannerPair> ScannerPairShPtr;
	typedef std::list<ScannerPairShPtr> ScannerPairList;
	ScannerPairList scanners;

	LuaVariant createLuaObject(const std::string& typeName, const void* pointer) const;
	bool getLuaObject(const LuaVariant& object, const std::string& typeName, void* &pointer) const;
	bool getScannerPair(const LuaVariant& object, ScannerPairList::const_iterator &iterator) const;

	ScannerPairShPtr getArgAsScannerObject(const std::vector<LuaVariant>& args) const;

	const ScanVariant getScanVariantFromLuaVariant(const LuaVariant &variant, const ScanVariant::ScanVariantType &type, bool allowBlank) const;
	LuaVariant getLuaVariantFromScanVariant(const ScanVariant &variant) const;
};
typedef std::shared_ptr<LuaEngine> LuaEngineShPtr;


class __LuaEngineExporter
{
public:
	__LuaEngineExporter(const char* name, const LuaVariant& value)
	{
		__luaEngineExports.push_back(std::make_pair(name, [=]() -> const LuaVariant { return value; }));
	}
	~__LuaEngineExporter(){}
};

template<typename K, typename A>
class __LuaEngineFactoryKeyExporter
{
public:
	__LuaEngineFactoryKeyExporter(const char* name, const KeyedFactory<K, A>& factory)
	{
		auto func = [&factory]() -> const LuaVariant
		{
			auto keys = factory.getKeys();
			LuaVariant::LuaVariantITable lkeys;
			for (auto key = keys.cbegin(); key != keys.cend(); key++)
				lkeys.push_back(*key);
			return lkeys;
		};
		__luaEngineExports.push_back(std::make_pair(name, func));
	}
	~__LuaEngineFactoryKeyExporter(){}
};