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

struct ScannerDataStructureDetails
{
	MemoryAddress identifier;
	std::map<std::string, ScanVariant> members;
};

typedef std::map<MemoryAddress, std::vector<MemoryAddress>> PointerMap;
typedef std::map<std::string, std::map<MemoryAddress, ScannerDataStructureDetails>> ScanDataStructureResultMap;

class ScannerDataStructureBlueprint
{
public:
	static const std::string ItemCountTag;
	static const std::string VFTableTag;

	virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		ScannerDataStructureDetails& details) const = 0;
	virtual std::string getTypeName() const = 0;

	virtual inline void findMatches(
		const ScannerTargetShPtr &target,
		const PointerMap &pointerMap,
		ScanDataStructureResultMap& results)
	{
		for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
		{
			ScannerDataStructureDetails details;
			if (this->walkStructure(target, ptrItr->first, pointerMap, details))
			{
				results[this->getTypeName()][details.identifier] = details;
			}
		}
	}

	static void findDataStructures(const ScannerTargetShPtr &target, const PointerMap &pointerMap, ScanDataStructureResultMap& results);
};

typedef KeyedFactory<std::string, ScannerDataStructureBlueprint> BPFactory;