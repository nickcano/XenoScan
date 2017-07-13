#pragma once

#include "Assert.h"
#include "ScannerTarget.h"
#include "ScannerTypes.h"
#include "ScanVariant.h"
#include "DataStructureBlueprint.h"


class StdMapBlueprint : public DataStructureBlueprint
{
public:
	static DataStructureBlueprint::FACTORY_TYPE::KEY_TYPE Key;

	inline virtual bool walkStructure(
		const ScannerTargetShPtr &target,
		const MemoryAddress &startPointer,
		const PointerMap &pointerMap,
		DataStructureDetails& details) const
	{
		if (this->findRootNode(target, startPointer, details))
		{			
			return this->countNodes(target, details);
		}

		return false;
	}
	virtual std::string getTypeName() const
	{
		return StdMapBlueprint::Key;
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

	inline bool findRootNode(const ScannerTargetShPtr &target, const MemoryAddress &startPointer, DataStructureDetails& details) const
	{
		size_t loops = 0;
		auto node = startPointer;

		// a map that is 50 levels high is just insane as fuck and
		// probably can't even exist so we're probably in a looping pointer chain
		// that looks like a map if that happens
		while (loops++ < 50)
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

	inline bool countNodes(const ScannerTargetShPtr &target, DataStructureDetails& details) const
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

		details.members.insert(std::make_pair(ItemCountTag, ScanVariant::FromNumber(searched.size())));
		return true;
	}
};