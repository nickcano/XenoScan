#pragma once
#include <memory>

#include "ScannerTypes.h"

template<typename CRTP>
class ScannerTargetHelper
{
public:
	template<typename T>
	inline T read(const MemoryAddress &adr) const
	{
		T ret;
		memset(&ret, 0, sizeof(T));
		static_cast<const CRTP*>(this)->rawRead(adr, sizeof(T), &ret);
		return ret;
	}

	template<typename T>
	inline bool read(const MemoryAddress &adr, T &value) const
	{
		return static_cast<const CRTP*>(this)->rawRead(adr, sizeof(T), &value);
	}

	template<typename T>
	inline bool readArray(const MemoryAddress &adr, const size_t size, T* &result) const
	{
		return static_cast<const CRTP*>(this)->rawRead(adr, sizeof(T) * size, &result[0]);
	}

	inline size_t getPointerSize() const
	{
		return static_cast<const CRTP*>(this)->pointerSize;
	}

	inline MemoryAddress incrementAddress(const MemoryAddress &adr, const size_t &times) const
	{
		return ((MemoryAddress)((size_t)adr + (times * static_cast<const CRTP*>(this)->pointerSize)));
	}
};