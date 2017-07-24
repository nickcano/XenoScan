#include "LuaEngine.h"

#include "XenoLua/LuaVariant.h"
#include "XenoScanEngine/ScannerTarget.h"
#include "XenoScanEngine/ScanVariant.h"
#include "XenoScanEngine/Scanner.h"

std::vector<std::pair<const std::string, const std::function<const LuaVariant()>>> __luaEngineExports;


// compare types
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_COMPARE_EQUALS,                     Scanner::SCAN_COMPARE_EQUALS);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_COMPARE_GREATER_THAN,               Scanner::SCAN_COMPARE_GREATER_THAN);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_COMPARE_GREATER_THAN_OR_EQUALS,     Scanner::SCAN_COMPARE_GREATER_THAN_OR_EQUALS);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_COMPARE_LESS_THAN,                  Scanner::SCAN_COMPARE_LESS_THAN);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_COMPARE_LESS_THAN_OR_EQUALS,        Scanner::SCAN_COMPARE_LESS_THAN_OR_EQUALS);

// scan types
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_INFER_TYPE_ALL_TYPES,               Scanner::SCAN_INFER_TYPE_ALL_TYPES);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_INFER_TYPE_STRING_TYPES,            Scanner::SCAN_INFER_TYPE_STRING_TYPES);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_INFER_TYPE_NUMERIC_TYPES,           Scanner::SCAN_INFER_TYPE_NUMERIC_TYPES);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_INFER_TYPE_EXACT,                   Scanner::SCAN_INFER_TYPE_EXACT);

// variant types
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_ASCII_STRING,               ScanVariant::SCAN_VARIANT_ASCII_STRING);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_WIDE_STRING,                ScanVariant::SCAN_VARIANT_WIDE_STRING);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_UINT8,                      ScanVariant::SCAN_VARIANT_UINT8);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_INT8,                       ScanVariant::SCAN_VARIANT_INT8);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_UINT16,                     ScanVariant::SCAN_VARIANT_UINT16);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_INT16,                      ScanVariant::SCAN_VARIANT_INT16);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_UINT32,                     ScanVariant::SCAN_VARIANT_UINT32);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_INT32,                      ScanVariant::SCAN_VARIANT_INT32);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_UINT64,                     ScanVariant::SCAN_VARIANT_UINT64);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_INT64,                      ScanVariant::SCAN_VARIANT_INT64);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_DOUBLE,                     ScanVariant::SCAN_VARIANT_DOUBLE);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_FLOAT,                      ScanVariant::SCAN_VARIANT_FLOAT);
LUAENGINE_EXPORT_VALUE(int32_t, SCAN_VARIANT_STRUCTURE,                  ScanVariant::SCAN_VARIANT_STRUCTURE);

// target keys
LUAENGINE_EXPORT_FACTORY_KEYS(ScannerTarget::FACTORY_TYPE, ScannerTarget::Factory, ATTACH_TARGET_NAMES);


LUAENGINE_EXPORT_FUNCTION(settimeout, "settimeout"); // settimeout(func, timeout)
int LuaEngine::settimeout()
{
	auto args = this->getArguments<LUA_VARIANT_FUNCTION_REF, LUA_VARIANT_INT>();

	LuaVariant::LuaVariantInt delay;
	args[1].getAsInt(delay);

	auto currentTime = std::chrono::high_resolution_clock::now();
	TimedEvent t;
	t.function = args[0];
	t.executeTime = currentTime + std::chrono::milliseconds(delay);

	this->timedEvents.push_back(t);
	
	return this->luaRet(true);
}


LUAENGINE_EXPORT_FUNCTION(attach, "attach"); // attach(type, pid)
int LuaEngine::attach()
{
	auto args = this->getArguments<LUA_VARIANT_STRING, LUA_VARIANT_INT>();

	std::string targetType;
	args[0].getAsString(targetType);
	ProcessIdentifier pid;
	args[1].getAsInt(pid);

	// create the target
	auto target = ScannerTarget::Factory.createInstance(targetType);
	if (!target)
		return this->luaRet();

	// attach to the target
	auto scannerPair = std::make_shared<ScannerPair>();
	scannerPair->target = target;
	if (!scannerPair->target->attach(pid))
		return this->luaRet();

	// if attach succeeded, create a scanner and push lua object
	scannerPair->scanner = std::make_shared<Scanner>();
	this->scanners.push_back(scannerPair);

	auto obj = this->createLuaObject("ScannerPair", scannerPair.get());
	return this->luaRet(obj);
}

LUAENGINE_EXPORT_FUNCTION(destroy, "destroy"); // destroy(scanner)
int LuaEngine::destroy()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE>();
	auto scanner = this->getArgAsScannerObject(args);
	if (scanner.get())
	{
		for (auto it = this->scanners.cbegin(); it != this->scanners.cend(); it++)
		{
			if (it->get() == scanner.get())
			{
				this->scanners.erase(it);
				return this->luaRet(true);
			}
		}
	}
	return this->luaRet(false);
}

LUAENGINE_EXPORT_FUNCTION(readMemory, "readMemory");
int LuaEngine::readMemory()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE, LUA_VARIANT_POINTER, LUA_VARIANT_INT, LUA_VARIANT_INT>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet();
	if (!scanner->target->isAttached()) return this->luaRet();

	MemoryAddress address;
	args[1].getAsPointer(address);

	LuaVariant::LuaVariantInt offset;
	args[2].getAsInt(offset);

	ScanVariant::ScanVariantType memberType;
	args[3].getAsInt(memberType);

	address = (MemoryAddress)((size_t)address + offset); // TODO need to clean this up when MemoryAddress operators are added
	auto result = ScanVariant::FromTargetMemory(scanner->target, address, memberType);
	return this->luaRet(this->getLuaVariantFromScanVariant(result));
}

LUAENGINE_EXPORT_FUNCTION(writeMemory, "writeMemory");
int LuaEngine::writeMemory()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE, LUA_VARIANT_POINTER, LUA_VARIANT_INT, LUA_VARIANT_STRING, LUA_VARIANT_INT>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);
	if (!scanner->target->isAttached()) return this->luaRet(false);

	MemoryAddress address;
	args[1].getAsPointer(address);

	LuaVariant::LuaVariantInt offset;
	args[2].getAsInt(offset);

	ScanVariant::ScanVariantType memberType;
	args[4].getAsInt(memberType);

	auto writeVariant = this->getScanVariantFromLuaVariant(args[3], memberType, false);
	if (writeVariant.isNull())
		return this->luaRet(false);

	address = (MemoryAddress)((size_t)address + offset); // TODO need to clean this up when MemoryAddress operators are added
	return this->luaRet(writeVariant.writeToTarget(scanner->target, address));
}


LUAENGINE_EXPORT_FUNCTION(newScan, "newScan");
int LuaEngine::newScan()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);
	if (!scanner->target->isAttached()) return this->luaRet(false);

	scanner->scanner->startNewScan();
	return this->luaRet(true);
}

LUAENGINE_EXPORT_FUNCTION(runScan, "runScan");
int LuaEngine::runScan()
{
	auto args = this->getArguments
				<
					LUA_VARIANT_KTABLE, // proc object
					LUA_VARIANT_KTABLE, // scan value
					LUA_VARIANT_INT,    // scan type
					LUA_VARIANT_INT,    // scan type mode
					LUA_VARIANT_INT     // scan comparator
				>();

	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);
	if (!scanner->target->isAttached()) return this->luaRet(false); 

	auto needle = ScanVariant::MakeNull();

	uint32_t type, typeMode, comparator;
	args[2].getAsInt(type);
	args[3].getAsInt(typeMode);
	args[4].getAsInt(comparator);

	LuaVariant::LuaVariantKTable valueTable;
	if (!args[1].getAsKTable(valueTable)) return this->luaRet(false);

	if (type == ScanVariant::SCAN_VARIANT_STRUCTURE)
	{
		// TODO: can pull this out into a function and
		// make recursive, though not very useful.

		// the structure, before it becomes a variant,
		// will end up in here
		std::vector<ScanVariant> members;

		// look up the schema table, which contains
		// the list of objects (with name and type)
		// in the proper order
		auto itSchema = valueTable.find("__schema");
		if (itSchema == valueTable.end()) return this->luaRet(false, "Expected '__schema' field in value table!");

		LuaVariant::LuaVariantITable schema;
		if (!itSchema->second.getAsITable(schema)) return this->luaRet(false, "Expected schema list to be an array!"); 

		// parse each member in the schema
		for (auto schemaEntry = schema.begin(); schemaEntry != schema.end(); schemaEntry++)
		{
			LuaVariant::LuaVariantKTable entry;
			if (!schemaEntry->getAsKTable(entry)) continue;

			// each entry has a name and a type
			auto itName = entry.find("__name");
			auto itType = entry.find("__type");
			if (itName == entry.end()) return this->luaRet(false, "Expected '__name' field in schema entry!");
			if (itType == entry.end()) return this->luaRet(false, "Expected '__type' field in schema entry!");

			LuaVariant::LuaVariantString memberName;
			LuaVariant::LuaVariantInt memberType;
			if (!itName->second.getAsString(memberName)) return this->luaRet(false, "Expected string value for '__name' field!");
			if (!itType->second.getAsInt(memberType)) return this->luaRet(false, "Expected number value for '__type' field!");

			// once we have the name and type parsed,
			// look it up the value and add it to
			// the member list
			auto itValue = valueTable.find(memberName);
			if (itValue == valueTable.end()) return this->luaRet(false, std::string("Expected to find '" + memberName + "' in value table!"));

			auto member = this->getScanVariantFromLuaVariant(itValue->second, memberType, true);
			if (member.isNull())
				return this->luaRet(false, "Unable to handle member type!");

			members.push_back(member);
		}
		needle = ScanVariant::FromStruct(members);
	}
	else
	{
		/*
			either it is a specific primitive type, meaning
			we're going to know the exact type we want, or
			it is going to be a basic primitive type, meaning
			we're going to know the value, but search for
			all variations fo the basic type.

			For specific primitive types, we will see 
			`typeMode` as `SCAN_TYPE_EXACT` and `type` as
			the expected type. In this case, our needle will
			be the exact type we want.
			
			For a basic primitive type, `typeMode` will be
			either `SCAN_INFER_TYPE_ALL_TYPES`,
			`SCAN_INFER_TYPE_STRING_TYPES`, or
			`SCAN_INFER_TYPE_NUMERIC_TYPES`. `type`
			will be 0. In this case, our needle will be an
			ascii string; the scanner will expand it.
		*/
		if (typeMode != Scanner::SCAN_INFER_TYPE_EXACT)
			type = ScanVariant::SCAN_VARIANT_ASCII_STRING;

		auto it = valueTable.find("value");
		if (it == valueTable.end()) return this->luaRet(false, "Expected 'value' field!");

		needle = this->getScanVariantFromLuaVariant(it->second, type, false);
		if (needle.isNull())
			return this->luaRet(false, "Unable to handle member type!");
	}

	scanner->scanner->runScan
	(
		scanner->target,
		needle,
		comparator,
		typeMode
	);
	return this->luaRet(true);
}

LUAENGINE_EXPORT_FUNCTION(getScanResultsSize, "getScanResultsSize");
int LuaEngine::getScanResultsSize()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);
	if (!scanner->target->isAttached()) return this->luaRet(false);
	if (!scanner->scanner->scanState.get()) return this->luaRet(false);

	return this->luaRet(scanner->scanner->scanState->resultSize());
}

LUAENGINE_EXPORT_FUNCTION(getScanResults, "getScanResults");
int LuaEngine::getScanResults()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE, LUA_VARIANT_INT, LUA_VARIANT_INT>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);
	if (!scanner->target->isAttached()) return this->luaRet(false);
	if (!scanner->scanner->scanState.get()) return this->luaRet(false);

	uint32_t start, length;
	args[1].getAsInt(start);
	args[2].getAsInt(length);

	auto resultsLength = scanner->scanner->scanState->resultSize();
	if (start < 0 || length < 0 || start >= resultsLength || start + length > resultsLength)
		return this->luaRet(false, "Invalid result range!");

	auto startIterator = scanner->scanner->scanState->beginResult();
	auto endIterator = scanner->scanner->scanState->endResult();
	auto res = startIterator;

	for (size_t i = 0; i < start; i++, res++); // find n-th iterator. ugly cause map

	LuaVariant::LuaVariantKTable results;
	for (; length > 0; length--, res++)
	{
		LuaVariant::LuaVariantITable innerResults;
		for (auto ires = res->second.begin(); ires != res->second.end(); ires++)
		{
			LuaVariant::LuaVariantKTable innerResultType;
			innerResultType["type"] = LuaVariant(ires->getTypeName());

			auto res = this->getLuaVariantFromScanVariant(*ires);
			if (res.isTable())
				innerResultType["values"] = res;
			else
				innerResultType["value"] = res;

			innerResults.push_back(innerResultType);
		}

		auto key = this->getLuaVariantFromScanVariant(res->first->toVariant());
		key.coerceToPointer();
		results[key] = innerResults;
	}

	return this->luaRet(results);
}

LUAENGINE_EXPORT_FUNCTION(getDataStructures, "getDataStructures");
int LuaEngine::getDataStructures()
{
	auto args = this->getArguments<LUA_VARIANT_KTABLE, LUA_VARIANT_STRING>();
	auto scanner = this->getArgAsScannerObject(args);
	if (!scanner.get()) return this->luaRet(false);

	std::string type;
	args[1].getAsString(type);

	scanner->scanner->runDataStructureScan(scanner->target, type);

	auto results = scanner->scanner->scanState->foundDataStructures();

	LuaVariant::LuaVariantKTable luaResults;
	for (auto dataStructureType = results.begin(); dataStructureType != results.end(); dataStructureType++)
	{
		LuaVariant::LuaVariantITable innerLuaResults;
		for (auto dataStructureResult = dataStructureType->second.begin();
			dataStructureResult != dataStructureType->second.end();
			dataStructureResult++)
		{
			LuaVariant::LuaVariantKTable singleLuaResult;
			singleLuaResult["identifier"] = dataStructureResult->second.identifier;
			for (auto member = dataStructureResult->second.members.begin();
				member != dataStructureResult->second.members.end();
				member++)
			{
				singleLuaResult[member->first] = this->getLuaVariantFromScanVariant(member->second);
			}

			innerLuaResults.push_back(singleLuaResult);
		}
		luaResults[dataStructureType->first] = innerLuaResults;
	}

	return this->luaRet(luaResults);
}