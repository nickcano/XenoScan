#pragma once
#include <functional>
#include <vector>

template<typename T>
class IRangeList
{
public:
	virtual const void iterate(std::function<void(const T& type)> callback) const = 0;
};

template<typename T>
class RangeList : public IRangeList<T>
{
public:
	RangeList() : low(0), high(0) {}
	RangeList(const T& low, const T& high) : low(low), high(high) {}

	virtual const void iterate(std::function<void(const T& type)> callback) const
	{
		for (auto i = this->low; i < this->high; i++)
			callback(i);
	}
private:
	T low, high;
};

template<typename T>
class RangeListAggregate : public IRangeList<T>
{
public:
	RangeListAggregate() {}

	void add(const RangeList<T>& range)
	{
		this->ranges.push_back(range);
	}
	virtual const void iterate(std::function<void(const T& type)> callback) const
	{
		for (auto&& range : this->ranges)
			range.iterate(callback);
	}
private:
	std::vector<RangeList<T>> ranges;
};