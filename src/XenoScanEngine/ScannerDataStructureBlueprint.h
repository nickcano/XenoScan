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

class StdListBlueprint : public ScannerDataStructureBlueprint
{
public:
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
		return "std::list";
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


class StdMapBlueprint : public ScannerDataStructureBlueprint
{
public:
	inline virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		ScannerDataStructureDetails& details) const
	{
		if (this->findRootNode(target, startPointer, details))
		{			
			return this->countNodes(target, details);
		}

		return false;
	}
	virtual std::string getTypeName() const
	{
		return "std::map";
	}

private:
	inline void getNodeRelationships(
		const ScannerTargetShPtr &target,
		const MemoryAddress &node,
		MemoryAddress &left,
		MemoryAddress &parent,
		MemoryAddress &right) const
	{
		left = target->read<MemoryAddress>(node);
		parent = target->read<MemoryAddress>(target->incrementAddress(node, 1));
		right = target->read<MemoryAddress>(target->incrementAddress(node, 2));
	}

	inline MemoryAddress getNodeParent(
		const ScannerTargetShPtr &target,
		const MemoryAddress &node) const
	{
		return target->read<MemoryAddress>(target->incrementAddress(node, 1));
	}

	inline bool validateNode(const ScannerTargetShPtr &target, const MemoryAddress &startPointer) const
	{
		MemoryAddress nodeLeft, nodeParent, nodeRight;
		this->getNodeRelationships(target, startPointer, nodeLeft, nodeParent, nodeRight);

		// Cases handled:
		//   - Single-node tree
		//   - Two-node tree leaning right
		//   - Two-node tree leaning left
		//   - Bottom node of tree
		//   - Node where at least two of [left], [parent], and [right]
		//     point back to it as expected.


		// a tree node will always have non-null relationships
		if (!nodeLeft || !nodeRight || !nodeParent) return false;
		
		if (nodeLeft != nodeRight)
		{
			MemoryAddress parentLeft, parentParent, parentRight;
			this->getNodeRelationships(target, nodeParent, parentLeft, parentParent, parentRight);
			if (!parentLeft || !parentParent || !parentRight) return false;

			if (nodeLeft == nodeParent) 
			{
				// we could be in a two-node tree where only
				// [right] points to a valid node, but [left] 
				// and [parent] point to [root].
				// We can verify this by checking if
				// [root]->[right] == [right] &&
				// [root]->[left] == [node] &&
				return nodeRight == parentRight && startPointer == parentLeft;
			}
			else if (nodeRight == nodeParent)
			{
				// we could be in a two-node tree where only
				// [left] points to a valid node, but [right]
				// and [parent] point to [root].
				// [root]->[left] == [left] &&
				// [root]->[right] == [node] &&
				return nodeLeft == parentLeft && startPointer == parentRight;
			}
			else
			{
				size_t matches = 0;
				if (getNodeParent(target, nodeLeft) == startPointer) matches++;
				if (getNodeParent(target, nodeRight) == startPointer) matches++;
				if (parentLeft == startPointer || parentRight == startPointer) matches++;
				return (matches >= 2);
			}
		}
		else // must be that (nodeLeft == nodeRight)
		{
			if (nodeLeft == nodeParent)
			{
				// this could be a node in a single-node tree
				// where [left], [parent], and [right] all point to
				// [root]. We can verify this by checking if
				// [node]->[parent]->[parent] == [node]
				auto nodeGrandParent = getNodeParent(target, nodeParent);
				return nodeGrandParent == startPointer;
			}
			else
			{
				// this could be a node at the bottom of the tree, where both
				// [left] and [right] point to [root]. If this is the case,
				// we can validate that it is indeed [root] by checking if
				// it's grandparent is it's parent. That is to say
				// [node]->[right] == [node]->[left] &&
				// [node]->[right]->[parent] == [node]->[right]->[parent]->[parent]->[parent]
				auto rightParent = getNodeParent(target, nodeRight);
				auto rightGrandParent = getNodeParent(target, rightParent);
				auto rightGreatGrandParent = getNodeParent(target, rightGrandParent);
				return rightParent && rightGrandParent && (rightGreatGrandParent == rightParent);
			}
		}
		return false;
	}

	inline bool findRootNode(const ScannerTargetShPtr &target, const MemoryAddress &startPointer, ScannerDataStructureDetails& details) const
	{
		size_t loops = 0;
		auto node = startPointer;

		// a map that is 200 levels high is just insane as fuck and
		// probably can't even exist so we're probably in a looping pointer chain
		// that looks like a map if that happens
		while (loops++ < 200)
		{
			if (!this->validateNode(target, node)) break;

			auto nodeParent = getNodeParent(target, node);
			auto nodeGrandParent = getNodeParent(target, nodeParent);
			if (!nodeParent || !nodeGrandParent) break;

			if (node == nodeGrandParent)
			{
				details.identifier = nodeParent;
				return true;
			}

			node = nodeParent;
		}
		return false;
	}

	inline bool countNodes(const ScannerTargetShPtr &target, ScannerDataStructureDetails& details) const
	{
		std::set<MemoryAddress> searched;
		std::stack<MemoryAddress> toSearch;
		toSearch.push(getNodeParent(target, details.identifier));
		while (toSearch.size() > 0)
		{
			auto search = toSearch.top();

			auto it = searched.find(search);
			if (it != searched.end())
				return false;

			if (!validateNode(target, search))
				return false;

			searched.insert(search);
			toSearch.pop();

			auto left = target->read<MemoryAddress>(search);
			if (left != details.identifier) toSearch.push(left);

			auto right = target->read<MemoryAddress>(target->incrementAddress(search, 2));
			if (right != details.identifier) toSearch.push(right);
		}

		details.members[ItemCountTag] = searched.size();
		return true;
	}
};


class ClassInstanceBlueprint : public ScannerDataStructureBlueprint
{
public:
	inline virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		ScannerDataStructureDetails& details) const
	{
		ASSERT(false); // NOT IMPLEMENTED BECAUSE findMatches DOES EVERYTHING
		return false;
	}

	virtual inline void findMatches(
		const ScannerTargetShPtr &target,
		const PointerMap &pointerMap,
		ScanDataStructureResultMap& results)
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
							ScannerDataStructureDetails details;
							details.identifier = instanceAddress;
							details.members[VFTableTag] = ptrItr->first;
							results[this->getTypeName()][instanceAddress] = details;
						}
					}
				}
			}
		}

	}


	virtual std::string getTypeName() const
	{
		return "Class Instance";
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