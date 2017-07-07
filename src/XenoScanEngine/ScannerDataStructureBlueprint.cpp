#include "ScannerDataStructureBlueprint.h"

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

const std::string ScannerDataStructureBlueprint::ItemCountTag = "itemCount";
const std::string ScannerDataStructureBlueprint::VFTableTag = "VFTable";

const std::string StdListBlueprint::Key = "std::list";
const std::string StdMapBlueprint::Key = "std::map";
const std::string NativeClassInstanceBlueprint::Key = "Native Class Instance";

BPFactory factory;
ADD_PRODUCER(BPFactory, factory, StdListBlueprint);
ADD_PRODUCER(BPFactory, factory, StdMapBlueprint);
ADD_PRODUCER(BPFactory, factory, NativeClassInstanceBlueprint);

void ScannerDataStructureBlueprint::findDataStructures(const ScannerTargetShPtr &target, const PointerMap &pointerMap, ScanDataStructureResultMap& results)
{
	auto& bps = target->getSupportedBlueprints();

	for (auto bp = bps.begin(); bp != bps.end(); bp++)
	{
		auto print = factory.createInstance(*bp);
		ASSERT(print != nullptr);
		print->findMatches(target, pointerMap, results);
	}
}