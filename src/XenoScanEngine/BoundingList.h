#pragma once
#include <memory>
#include <list>
#include <algorithm>


template <typename T, typename C>
class BoundingList
{
public:
	bool insert(const T& begin, const T& end)
	{
		auto target = std::tuple<T, T>(begin, end);

		// we're using a function called "std::lower_bound", but it will actually
		// return the first element NOT LOWER than the target element.
		auto lower = std::lower_bound(
			this->bounds.begin(), this->bounds.end(),
			target, C::CompareBounds
		);

		if (lower == this->bounds.end())
			this->bounds.push_back(target); // doesn't overlap with anything, add to end
		else if (!C::CompareBounds(target, *lower))
		{
			// the search calls the comparator as `C(*lower, target)` and we check
			// `!C(*lower, target)`. together, these checks mean "these are 
			// overlapping", so we coalesce these into a single block and keep the
			// newest data.
			auto lowest = std::min(std::get<0>(target), std::get<0>(*lower));
			auto highest = std::max(std::get<1>(target), std::get<1>(*lower));
			*lower = std::make_tuple(lowest, highest);
			return false;
		}
		else
		{
			// this can be an equal element, but when it's not, it's the element
			// before which we should insert the target to keep our ordering correct.
			this->bounds.insert(lower, target);
		}

		return true; // true means "newly inserted"
	}


	bool contains(const T& begin, const T& end) const
	{
		auto target = std::tuple<T, T>(begin, end);
		auto lower = std::lower_bound(
			this->bounds.cbegin(), this->bounds.cend(),
			target, C::CompareBounds
		);
		if (lower == this->bounds.cend())
			return false; // nothing above
		else if (!C::CompareBounds(target, *lower))
			return true;  // !(lower < target) && !(target < lower)
		return false;     // !(lower < target) &&  (target < lower)
	}

	void clear()
	{
		this->bounds.clear();
	}

private:
	std::list<std::tuple<T, T>> bounds;
};

