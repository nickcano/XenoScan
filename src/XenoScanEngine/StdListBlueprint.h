#pragma once

#include "Assert.h"
#include "ScannerTarget.h"
#include "ScannerTypes.h"
#include "ScanVariant.h"
#include "ScannerDataStructureBlueprint.h"

class StdListBlueprint : public ScannerDataStructureBlueprint
{
public:
	static const std::string Key;

	inline virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		ScannerDataStructureDetails& details) const
	{
		if (this->validateNode(target, startPointer))
		{
			// Assume circular list, loop forward to count objects
			std::vector<MemoryAddress> objects;
			MemoryAddress nextObject = startPointer;
			do
			{
				objects.push_back(nextObject);
				nextObject = target->read<MemoryAddress>(nextObject);
				if (!this->validateNode(target, nextObject))
					return false;
			}
			while (nextObject != startPointer);

			// don't waste time on empty lists
			if (objects.size() <= 1)
				return false;

			// Now that objects are counted, locate the root node. It will look like:
			//     <pointer to some list object>
			//     <size of that list>
			// So just check for the size, since the pointer map is keyed on address.
			// We'll stop on the first match, assuming it's really the root node.
			auto walkedCount = objects.size() - 1; // subtract 1 because null marker not included
			for (auto object = objects.cbegin(); object != objects.cend(); object++)
			{
				auto located = pointerMap.find(*object);
				if (located != pointerMap.end())
				{
					for (auto ref = located->second.cbegin(); ref != located->second.cend(); ref++)
					{
						auto sizeLocation = target->incrementAddress(*ref, 1);
						auto size = target->read<size_t>(sizeLocation);
						if (size == walkedCount)
						{
							details.identifier = *object;
							details.members[ItemCountTag] = size;
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	virtual std::string getTypeName() const
	{
		return StdListBlueprint::Key;
	}

private:
	inline bool validateNode(const ScannerTargetShPtr &target, const MemoryAddress &startPointer) const
	{
		// validate the node by verifying that there's both:
		//   - An object following it which points back to it
		//   - And object before it which points forward to it
		auto nextObject = target->read<MemoryAddress>(startPointer);
		auto previousObject = target->read<MemoryAddress>(target->incrementAddress(startPointer, 1));
		auto nextObjectBack = target->read<MemoryAddress>(target->incrementAddress(nextObject, 1));
		auto previousObjectForward = target->read<MemoryAddress>(previousObject);

		return (startPointer == nextObjectBack && startPointer == previousObjectForward);
	}
};