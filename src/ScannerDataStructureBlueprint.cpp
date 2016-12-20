#include "ScannerDataStructureBlueprint.h"

/*
	The job of the code in this file is to detect different
	complex data structures within the target's memory. A
	lot of helper code is in ScannerDataStructureBlueprint.h.

	So far, we can detect:
		1. std::list (or any double-linked list that is circular and is pointed down to by a root node)
		2. std::map (or any balanced tree where the first node's paren't points to a root, whose parent points to the first node)
		3. abstract class instances (anything with a VF table)
*/


// TODO: maybe clean this up?
inline void findClassInstances(const ScannerTargetShPtr &target, const PointerMap &pointerMap, ScanDataStructureResultMap& results)
{
	auto pageSize = target->pageSize();

	// identify all the different blocks in the main module of the target
	//     E.G. on windows this will be the regions in PE header with
	//     different protections
	std::vector<MemoryInformation> executableBlocks, readOnlyBlocks;
	MemoryAddress moduleStart, moduleEnd, nextAddress;
	if (target->getMainModuleBounds(moduleStart, moduleEnd))
	{
		nextAddress = moduleStart;
		while (nextAddress < moduleEnd)
		{
			MemoryInformation meminfo;
			if (target->queryMemory(nextAddress, meminfo) && meminfo.isCommitted)
			{
				nextAddress = (MemoryAddress)((size_t)meminfo.allocationBase + meminfo.allocationSize + pageSize);
				if (meminfo.isExecutable)
					executableBlocks.push_back(meminfo);
				else if (!meminfo.isWriteable)
					readOnlyBlocks.push_back(meminfo);
			}
			else
				nextAddress = (MemoryAddress)((size_t)nextAddress + pageSize);
		}
	}

	// some helper lambdas
	auto isInExecutableBlock = [&executableBlocks](const MemoryAddress &adr) -> bool
	{
		for (auto b = executableBlocks.begin(); b != executableBlocks.end(); b++)
			if (adr >= b->allocationBase && adr < b->allocationEnd)
				return true;
		return false;
	};
	auto isInReadOnlyBlock = [&readOnlyBlocks](const MemoryAddress &adr) -> bool
	{
		for (auto b = readOnlyBlocks.begin(); b != readOnlyBlocks.end(); b++)
			if (adr >= b->allocationBase && adr < b->allocationEnd)
				return true;
		return false;
	};
	
	// do it
	for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
	{
		// There's a pointer to read only memory, maybe a VF table
		if (isInReadOnlyBlock(ptrItr->first))
		{
			auto pointed = target->read<MemoryAddress>(ptrItr->first);

			// The thing in read-only memory points to executable memory, definitely a VF table
			if (isInExecutableBlock(pointed))
			{
				for (auto instance = ptrItr->second.begin(); instance != ptrItr->second.end(); instance++)
				{
					auto instanceAddress = *instance;
					if (instanceAddress < moduleStart ||  instanceAddress > moduleEnd)
					{
						ScannerDataStructureDetails details;
						details.rootNode = ptrItr->first;
						details.objectCount = (size_t)instanceAddress;
						results["Class Instance"][instanceAddress] = details;
					}
				}
			}
		}
	}

}

// the reason we're generating a new function for each blueprint,
// rather than using a list of ScannerDataStructureBlueprint
// pointers, is to take advantage of the function inlining
// we gain by knowing the exact type and not relying on the vtable.
template<typename BLUEPRINT>
inline void findBlueprint(const ScannerTargetShPtr &target, const PointerMap &pointerMap, ScanDataStructureResultMap& results)
{
	BLUEPRINT blueprint;
	for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
	{
		ScannerDataStructureDetails details;
		if (blueprint.walkStructure(target, ptrItr->first, pointerMap, details))
		{
			results[blueprint.getTypeName()][details.rootNode] = details;
		}
	}
}

void ScannerDataStructureBlueprint::findDataStructures(const ScannerTargetShPtr &target, const PointerMap &pointerMap, ScanDataStructureResultMap& results)
{
	//findBlueprint<StdListBlueprint>(target, pointerMap, results);
	//findBlueprint<StdMapBlueprint>(target, pointerMap, results);
	findClassInstances(target, pointerMap, results);
}