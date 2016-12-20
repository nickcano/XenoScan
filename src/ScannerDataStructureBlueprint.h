#pragma once
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <set>

#include "ScannerTarget.h"
#include "ScannerTypes.h"

struct ScannerDataStructureDetails
{
	MemoryAddress rootNode;
	size_t objectCount;
};

typedef std::map<MemoryAddress, std::vector<MemoryAddress>> PointerMap;
typedef std::map<std::string, std::map<MemoryAddress, ScannerDataStructureDetails>> ScanDataStructureResultMap;

class ScannerDataStructureBlueprint
{
public:
	virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		ScannerDataStructureDetails& details) const = 0;
	virtual std::string getTypeName() const = 0;

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
						auto sizeLocation = OFFSET_MEMORY_ADDRESS(*ref, sizeof(MemoryAddress));
						auto size = target->read<size_t>(sizeLocation);
						if (size == walkedCount)
						{
							details.rootNode = *object;
							details.objectCount = size;
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
		auto previousObject = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(startPointer, sizeof(MemoryAddress)));
		auto nextObjectBack = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(nextObject, sizeof(MemoryAddress)));
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
		if (this->findRootNode(target, startPointer, details.rootNode))
		{
			// TODO: fix counting of tree nodes .. fuck
			// some thoughts:
			//     - This could be messing up because the process isn't forzen
			//     - other than that, probably fml
			
			return this->countNodes(target, details.rootNode, details.objectCount);
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
		left = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(node, 0));
		parent = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(node, sizeof(MemoryAddress)));
		right = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(node, sizeof(MemoryAddress) * 2));
	}

	inline MemoryAddress getNodeParent(
		const ScannerTargetShPtr &target,
		const MemoryAddress &node) const
	{
		return target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(node, sizeof(MemoryAddress)));
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

	inline bool findRootNode(const ScannerTargetShPtr &target, const MemoryAddress &startPointer, MemoryAddress &root) const
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
				root = nodeParent;
				return true;
			}

			node = nodeParent;
		}
		return false;
	}

	inline bool countNodes(const ScannerTargetShPtr &target, const MemoryAddress &root, size_t &count) const
	{
		std::set<MemoryAddress> searched;
		std::stack<MemoryAddress> toSearch;
		toSearch.push(getNodeParent(target, root));
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
			if (left != root) toSearch.push(left);

			auto right = target->read<MemoryAddress>(OFFSET_MEMORY_ADDRESS(search, sizeof(MemoryAddress) * 2));
			if (right != root) toSearch.push(right);
		}

		count = searched.size();
		return true;
	}
};