#include "ScannerTargetWindows.h"

#include "Assert.h"
#include "StdListBlueprint.h"
#include "StdMapBlueprint.h"
#include "NativeClassInstanceBlueprint.h"

#include <Windows.h>
#include <Psapi.h>

#include <tlhelp32.h>

#define WIN32_IS_EXECUTABLE_PROT(x) (x == PAGE_EXECUTE || x == PAGE_EXECUTE_READ || x == PAGE_EXECUTE_READWRITE || x == PAGE_EXECUTE_WRITECOPY)
#define WIN32_IS_WRITEABLE_PROT(x) (x == PAGE_EXECUTE_READWRITE || x == PAGE_READWRITE)

ScannerTargetWindows::ScannerTargetWindows() :
	processHandle(NULL)
{
	this->supportedBlueprints.insert(StdListBlueprint::Key);
	this->supportedBlueprints.insert(StdMapBlueprint::Key);
	this->supportedBlueprints.insert(NativeClassInstanceBlueprint::Key);

	this->pointerSize = sizeof(void*);
	this->littleEndian = true;

	static_assert(sizeof(void*) <= sizeof(MemoryAddress), "MemoryAddress type is too small!");
}

ScannerTargetWindows::~ScannerTargetWindows()
{
	if (this->processHandle)
	{
		CloseHandle(reinterpret_cast<HANDLE>(this->processHandle));
		this->processHandle = NULL;
	}
}

bool ScannerTargetWindows::attach(const ProcessIdentifier &pid)
{
	// detach if we're attached to something else
	if (this->isAttached())
	{
		CloseHandle(this->processHandle);
		this->processHandle = NULL;
	}

	// some safety checks
	static_assert(sizeof(pid) == sizeof(DWORD), "Expected size of pid to match size of DWORD on Windows");
	static_assert(sizeof(this->processHandle) == sizeof(HANDLE), "Expected size of this->processHandle to match size of HANDLE on Windows");

	// open the process
	DWORD _pid = static_cast<DWORD>(pid);
	auto handle = OpenProcess(
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD,
		FALSE,
		_pid
	);

	if (!handle)
		return false;

	this->pid = pid;
	this->processHandle = reinterpret_cast<ProcessHandle>(handle);

	// find the main module bounds
	this->buildModuleBounds();

	// find the system info
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	static_assert(sizeof(this->highestAddress) == sizeof(sysinfo.lpMaximumApplicationAddress), "Expected SYSTEM_INFO structure to have addresses the same size as scanner's MemoryAddress type");

	this->pageSize = static_cast<size_t>(sysinfo.dwPageSize);
	this->highestAddress = reinterpret_cast<MemoryAddress>(sysinfo.lpMaximumApplicationAddress);
	this->lowestAddress = reinterpret_cast<MemoryAddress>(sysinfo.lpMinimumApplicationAddress);

	// we good!
	return true;
}

bool ScannerTargetWindows::isAttached() const
{
	return (this->processHandle != 0);
}

bool ScannerTargetWindows::queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const
{
	ASSERT(this->isAttached());

	MEMORY_BASIC_INFORMATION memoryInfo;

	static_assert(sizeof(meminfo.allocationBase) == sizeof(memoryInfo.AllocationBase), "Expected size of meminfo.allocationBase to match size of memoryInfo.AllocationBase on Windows");
	static_assert(sizeof(meminfo.allocationSize) == sizeof(memoryInfo.RegionSize), "Expected size of meminfo.allocationSize to match size of memoryInfo.RegionSize on Windows");

	if (!VirtualQueryEx(this->processHandle, adr, &memoryInfo, sizeof(memoryInfo)))
	{
		nextAdr = (MemoryAddress)((size_t)adr + this->pageSize);
		return false;
	}
	
	meminfo.isMirror = false;
	meminfo.isCommitted =    (memoryInfo.State == MEM_COMMIT);
	meminfo.allocationBase = memoryInfo.BaseAddress;
	meminfo.allocationSize = memoryInfo.RegionSize;
	meminfo.allocationEnd =  (MemoryAddress)((size_t)meminfo.allocationBase + meminfo.allocationSize);
	meminfo.isModule =       this->isWithinModule(meminfo.allocationBase, meminfo.allocationEnd);
	meminfo.isMappedImage =  (memoryInfo.Type == MEM_IMAGE);
	meminfo.isMapped =       (memoryInfo.Type == MEM_MAPPED);

	meminfo.isExecutable = WIN32_IS_EXECUTABLE_PROT(memoryInfo.Protect);
	meminfo.isWriteable = WIN32_IS_WRITEABLE_PROT(memoryInfo.Protect);

	nextAdr = meminfo.allocationEnd;
	return true;
}

bool ScannerTargetWindows::isWithinModule(MemoryAddress &start, MemoryAddress &end) const
{
	return this->moduleBounds.contains(start, end);
}

bool ScannerTargetWindows::getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const
{
	start = this->mainModuleStart;
	end = this->mainModuleEnd;
	return true;
}

uint64_t ScannerTargetWindows::getFileTime64() const
{
	FILETIME time;
	GetSystemTimeAsFileTime(&time);

	uint64_t ret;
	static_assert(sizeof(ret) <= sizeof(time), "FILETIME must be able to fill uint64_t!");
	memcpy(&ret, &time, sizeof(ret));
	return ret;
}

uint32_t ScannerTargetWindows::getTickTime32() const
{
	return static_cast<uint32_t>(GetTickCount());
}

bool ScannerTargetWindows::rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const
{
	ASSERT(this->isAttached());
	return (ReadProcessMemory(this->processHandle, adr, result, objectSize, NULL) != 0);
}

bool ScannerTargetWindows::rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const
{
	ASSERT(this->isAttached());
	return (WriteProcessMemory(this->processHandle, adr, data, objectSize, NULL) != 0);
}

void ScannerTargetWindows::buildModuleBounds()
{
	// WARNING: not thread safe for any updated members
	this->moduleBounds.clear();

	auto type = (sizeof(MemoryAddress) == 4)
		? TH32CS_SNAPMODULE // if we're 32bit, just query "native modules"
		: (TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE); // if we're 64bit, make sure to grab both 64bit ("native") and 32bit modules

	MODULEENTRY32W entry;
	entry.dwSize = sizeof(MODULEENTRY32W);

	auto snapshot = CreateToolhelp32Snapshot(type, this->pid);
	if (Module32FirstW(snapshot, &entry) == TRUE)
	{
		// currently treating the first module as the main module.
		// havent yet validated that this assumption is always true.
		this->mainModuleStart = reinterpret_cast<MemoryAddress>(entry.modBaseAddr);
		this->mainModuleEnd = reinterpret_cast<MemoryAddress>(&entry.modBaseAddr[entry.modBaseSize]);

		do
		{
			//wprintf(L"%s\n\t%s\n", entry.szExePath, entry.szModule);
			this->moduleBounds.insert(
				reinterpret_cast<MemoryAddress>(entry.modBaseAddr),
				reinterpret_cast<MemoryAddress>(&entry.modBaseAddr[entry.modBaseSize])
			);
		}
		while (Module32NextW(snapshot, &entry) == TRUE);
	}
	CloseHandle(snapshot);
}