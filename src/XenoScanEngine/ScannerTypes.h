#pragma once

#include <stdint.h>
#include <vector>

#define OFFSET_MEMORY_ADDRESS(a, offset) ((MemoryAddress)((size_t)a + offset))

typedef void* MemoryAddress;
typedef uint32_t ProcessIdentifier;
typedef void* ProcessHandle;
typedef uint32_t MemoryAccessRights;

typedef uint32_t CompareTypeFlags;

struct MemoryInformation
{
	bool isCommitted, isWriteable, isExecutable;
	MemoryAddress allocationBase, allocationEnd;
	size_t allocationSize;
};

typedef std::vector<MemoryInformation> MemoryInformationCollection;