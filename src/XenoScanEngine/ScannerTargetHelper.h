#pragma once
#include <memory>
#include <string>

#include "ScannerTypes.h"
#include "KeyedFactory.h"

template<typename CRTP>
class ScannerTargetHelper
{
public:
	typedef KeyedFactory<std::string, CRTP> FACTORY_TYPE;

	template<typename T>
	inline bool write(const MemoryAddress &adr, const T &value) const
	{
		return static_cast<const CRTP*>(this)->rawWrite(adr, sizeof(T), &value);
	}

	template<typename T>
	inline bool writeArray(const MemoryAddress &adr, const size_t size, const T* value) const
	{
		return static_cast<const CRTP*>(this)->rawWrite(adr, sizeof(T) * size, &value[0]);
	}

	inline bool writeString(const MemoryAddress &adr, const std::string &value) const
	{
		auto size = value.length() + 1;
		return static_cast<const CRTP*>(this)->rawWrite(adr, size * sizeof(std::string::value_type), &value.c_str()[0]);
	}

	inline bool writeString(const MemoryAddress &adr, const std::wstring &value) const
	{
		auto size = value.length() + 1;
		return static_cast<const CRTP*>(this)->rawWrite(adr, size * sizeof(std::wstring::value_type), &value.c_str()[0]);
	}


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

	inline bool readString(const MemoryAddress &adr, std::string &result) const
	{
		size_t finalLength;
		auto res = readTerminatedBuffer<std::string::value_type>(adr, 0x00, finalLength);
		if (!res)
			return false;
		result = std::string(res, &res[finalLength]);
		delete[] res;
		return true;
	}

	inline bool readString(const MemoryAddress &adr, std::wstring &result) const
	{
		size_t finalLength;
		auto res = readTerminatedBuffer<std::wstring::value_type>(adr, 0x0000, finalLength);
		if (!res)
			return false;
		result = std::wstring(res, &res[finalLength]);
		delete[] res;
		return true;
	}

	inline size_t getPointerSize() const
	{
		return static_cast<const CRTP*>(this)->pointerSize;
	}

	inline MemoryAddress getLowestAddress() const
	{
		return static_cast<const CRTP*>(this)->lowestAddress;
	}

	inline MemoryAddress getHighestAddress() const
	{
		return static_cast<const CRTP*>(this)->highestAddress;
	}

	inline bool isLittleEndian() const
	{
		return static_cast<const CRTP*>(this)->littleEndian;
	}

	inline const std::set<std::string>& getSupportedBlueprints() const
	{
		return static_cast<const CRTP*>(this)->supportedBlueprints;
	}

	inline MemoryAddress incrementAddress(const MemoryAddress &adr, const size_t &times) const
	{
		return ((MemoryAddress)((size_t)adr + (times * static_cast<const CRTP*>(this)->pointerSize)));
	}

private:
	template<typename T>
	inline T* readTerminatedBuffer(const MemoryAddress &adr, const T &terminator, size_t &length) const
	{
		// get the size of the entire memory block
		MemoryInformation memInfo;
		MemoryAddress next;
		auto result = static_cast<const CRTP*>(this)->queryMemory(adr, memInfo, next);
		if (!result)
			return nullptr;

		// read the whole thing starting at our target address
		length = ((size_t)memInfo.allocationEnd - (size_t)adr) / sizeof(T);
		auto buffer = new T[length];
		if (!this->readArray(adr, length, buffer))
		{
			delete[] buffer;
			return nullptr;
		}

		// find the first instance of the terminator
		for (size_t i = 0; i < length; i++)
		{
			if (buffer[i] == terminator)
			{
				length = i + 1;
				break;
			}
		}

		// we're done
		return buffer;
	}

	virtual bool queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const = 0;
};
