#include "DataStructureBlueprint.h"

#include "StdListBlueprint.h"
#include "StdMapBlueprint.h"
#include "NativeClassInstanceBlueprint.h"

/*
	The job of the code in this file is to detect different
	complex data structures within the target's memory. A
	lot of helper code is in ScannerDataStructureBlueprint.h.

	So far, we can detect:
		1. std::list (or any double-linked list that is circular and is pointed down to by a root node)
		2. std::map (or any balanced tree where the first node's paren't points to a root, whose parent points to the first node)
		3. abstract class instances (anything with a VF table)
*/

const std::string DataStructureBlueprint::ItemCountTag = "itemCount";
const std::string DataStructureBlueprint::VFTableTag = "VFTable";

CREATE_FACTORY(DataStructureBlueprint);
CREATE_PRODUCER(DataStructureBlueprint, StdListBlueprint,              "std::list");
CREATE_PRODUCER(DataStructureBlueprint, StdMapBlueprint,               "std::map");
CREATE_PRODUCER(DataStructureBlueprint, NativeClassInstanceBlueprint,  "Native Class Instance");


void DataStructureBlueprint::findDataStructures(const ScannerTargetShPtr &target, const DataStructureBlueprint::FACTORY_TYPE::KEY_TYPE &key, const PointerMap &pointerMap, DataStructureResultMap& results)
{
	auto supported = target->getSupportedBlueprints();
	if (supported.find(key) == supported.cend())
		return;

	auto print = DataStructureBlueprint::Factory.createInstance(key);
	ASSERT(print != nullptr);
	print->findMatches(target, pointerMap, results);
}