#pragma once

#include "Assert.h"
#include "ScannerTarget.h"
#include "ScannerTypes.h"
#include "ScanVariant.h"
#include "DataStructureBlueprint.h"

class NativeClassInstanceBlueprint : public DataStructureBlueprint
{
public:
	static const std::string Key;

	inline virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		DataStructureDetails& details) const
	{
		ASSERT(false); // NOT IMPLEMENTED BECAUSE findMatches DOES EVERYTHING
		return false;
	}

	virtual inline void findMatches(
		const ScannerTargetShPtr &target,
		const PointerMap &pointerMap,
		DataStructureResultMap& results)
	{
		MemoryAddress moduleStart, moduleEnd;
		if (!target->getMainModuleBounds(moduleStart, moduleEnd))
			return;

		std::vector<MemoryInformation> executableBlocks, readOnlyBlocks;
		this->getBlocks(target, moduleStart, moduleEnd, executableBlocks, readOnlyBlocks);

		for (auto ptrItr = pointerMap.cbegin(); ptrItr != pointerMap.cend(); ptrItr++)
		{
			// There's a pointer to read only memory, maybe a VF table
			if (this->isInBlock(readOnlyBlocks, ptrItr->first))
			{
				auto pointed = target->read<MemoryAddress>(ptrItr->first);

				// The thing in read-only memory points to executable memory, definitely a VF table
				if (this->isInBlock(executableBlocks, pointed))
				{
					for (auto instance = ptrItr->second.begin(); instance != ptrItr->second.end(); instance++)
					{
						auto instanceAddress = *instance;
						if (instanceAddress < moduleStart ||  instanceAddress > moduleEnd)
						{
							DataStructureDetails details;
							details.identifier = instanceAddress;
							details.members.insert(std::make_pair(VFTableTag, ScanVariant::FromMemoryAddress(ptrItr->first)));
							results[this->getTypeName()][instanceAddress] = details;
						}
					}
				}
			}
		}

	}

	virtual std::string getTypeName() const
	{
		return NativeClassInstanceBlueprint::Key;
	}

private:
	inline bool isInBlock(const std::vector<MemoryInformation> &blocks, const MemoryAddress &adr)
	{
		for (auto b = blocks.begin(); b != blocks.end(); b++)
			if (adr >= b->allocationBase && adr < b->allocationEnd)
				return true;
		return false;
	}

	inline void getBlocks(
		const ScannerTargetShPtr &target,
		const MemoryAddress &moduleStart,
		const MemoryAddress &moduleEnd,
		std::vector<MemoryInformation> &executableBlocks,
		std::vector<MemoryInformation> &readOnlyBlocks) const
	{
		// identify all the different blocks in the main module of the target
		//     E.G. on windows this will be the regions in PE header with
		//     different protections
		MemoryAddress nextAddress = moduleStart;
		while (nextAddress < moduleEnd)
		{
			MemoryInformation meminfo;
			if (target->queryMemory(nextAddress, meminfo, nextAddress) && meminfo.isCommitted)
			{
				if (meminfo.isExecutable)
					executableBlocks.push_back(meminfo);
				else if (!meminfo.isWriteable)
					readOnlyBlocks.push_back(meminfo);
			}
		}
	}
};