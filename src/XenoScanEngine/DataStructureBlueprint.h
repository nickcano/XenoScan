#pragma once
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <set>
#include <functional>

#include "Assert.h"
#include "ScannerTarget.h"
#include "ScannerTypes.h"
#include "ScanVariant.h"
#include "KeyedFactory.h"

struct DataStructureDetails
{
	MemoryAddress identifier;
	// this will throw a compile error if we access the map using [],
	// as the default constructor is marked as private to prevent
	// implicitly getting null variants.
	std::map<std::string, ScanVariant> members;
};

typedef std::map<MemoryAddress, std::vector<MemoryAddress>> PointerMap;
typedef std::map<std::string, std::map<MemoryAddress, DataStructureDetails>> DataStructureResultMap;

class DataStructureBlueprint
{
public:
	typedef KeyedFactory<std::string, DataStructureBlueprint> FACTORY_TYPE;
	static FACTORY_TYPE Factory;


	static const std::string ItemCountTag;
	static const std::string VFTableTag;

	virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		DataStructureDetails& details) const = 0;
	virtual std::string getTypeName() const = 0;

	virtual inline void findMatches(
		const ScannerTargetShPtr &target,
		const PointerMap &pointerMap,
		DataStructureResultMap& results)
	{
		for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
		{
			DataStructureDetails details;
			if (this->walkStructure(target, ptrItr->first, pointerMap, details))
			{
				results[this->getTypeName()][details.identifier] = details;
			}
		}
	}

	static void findDataStructures(const ScannerTargetShPtr &target, const PointerMap &pointerMap, DataStructureResultMap& results);
};