#include "DataStructureBlueprint.h"

#include "StdListBlueprint.h"
#include "StdMapBlueprint.h"
#include "NativeClassInstanceBlueprint.h"

#include "ThreadPool.h"
#include "ConsoleProgressTracker.h"


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


void DataStructureBlueprint::findDataStructures(
	const ScannerTargetShPtr &target,
	const DataStructureBlueprint::FACTORY_TYPE::KEY_TYPE &key,
	const PointerMap &pointerMap,
	DataStructureResultMap& results)
{
	auto supported = target->getSupportedBlueprints();
	if (supported.find(key) == supported.cend())
		return;

	auto print = DataStructureBlueprint::Factory.createInstance(key);
	ASSERT(print != nullptr);
	print->findMatches(target, pointerMap, results);
}


void DataStructureBlueprint::findMatches(
	const ScannerTargetShPtr &target,
	const PointerMap &pointerMap,
	DataStructureResultMap& results)
{
	ThreadPool pool;
	std::mutex mutex;
	ConsoleProgressTracker tracker(
		"Pointer Tree",
		pool.getNumberOfWorkers(),
		pointerMap.size(),
		(pointerMap.size() / 100) + 1
	);

	for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
	{
		pool.execute([this, &pointerMap, &target, &results, &mutex, &ptrItr]() -> void {
			DataStructureDetails details;
			if (this->walkStructure(target, ptrItr->first, pointerMap, details))
			{
				mutex.lock();
				results[this->getTypeName()][details.identifier] = details;
				mutex.unlock();
			}
		});
	}

	pool.join([&pointerMap, &tracker](size_t remaining) -> void {
		tracker.setNumberOfCompleteTasks(pointerMap.size() - remaining);
	});
}