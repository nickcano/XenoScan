#pragma once
#include "ScannerTargetWindows.h"
#include "Assert.h"

#include <Windows.h>
#include <Psapi.h>

#define WIN32_IS_EXECUTABLE_PROT(x) (x == PAGE_EXECUTE || x == PAGE_EXECUTE_READ || x == PAGE_EXECUTE_READWRITE || x == PAGE_EXECUTE_WRITECOPY)
#define WIN32_IS_WRITEABLE_PROT(x) (x == PAGE_EXECUTE_READWRITE || x == PAGE_READWRITE)

ScannerTargetShPtr ScannerTarget::createScannerTarget()
{
	return ScannerTargetShPtr(new ScannerTargetWindows());
}

ScannerTargetWindows::ScannerTargetWindows() :
	processHandle(NULL)
{
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
	static_assert(sizeof(pid) == sizeof(DWORD), "Expected size ofpid to match size of DWORD on Windows");
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

	this->processHandle = reinterpret_cast<ProcessHandle>(handle);

	// find the main module bounds
	mainModuleStart = this->getMainModuleBaseAddress();
	if (mainModuleStart == 0)
		return false;

	MODULEINFO moduleInfo;
	if (!GetModuleInformation(this->processHandle, (HMODULE)mainModuleStart, &moduleInfo, sizeof(moduleInfo)))
		return false;

	mainModuleEnd = (MemoryAddress)((size_t)mainModuleStart + moduleInfo.SizeOfImage);

	// we good!
	return true;
}

bool ScannerTargetWindows::isAttached() const
{
	return (this->processHandle != 0);
}

MemoryAddress ScannerTargetWindows::lowestAddress() const
{
	//TODO: be smarter
	return reinterpret_cast<MemoryAddress>(0x0);
}
MemoryAddress ScannerTargetWindows::highestAddress() const
{
	//TODO: be smarter
	return reinterpret_cast<MemoryAddress>(0x07FFFFFF);
}
size_t ScannerTargetWindows::pageSize() const
{
	//TODO: get from system
	return 0x1000;
}
size_t ScannerTargetWindows::chunkSize() const
{
	return 0x800000;
}

bool ScannerTargetWindows::queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo) const
{
	ASSERT(this->isAttached());

	MEMORY_BASIC_INFORMATION memoryInfo;

	static_assert(sizeof(meminfo.allocationBase) == sizeof(memoryInfo.AllocationBase), "Expected size of meminfo.allocationBase to match size of memoryInfo.AllocationBase on Windows");
	static_assert(sizeof(meminfo.allocationSize) == sizeof(memoryInfo.RegionSize), "Expected size of meminfo.allocationSize to match size of memoryInfo.RegionSize on Windows");

	if (!VirtualQueryEx(this->processHandle, adr, &memoryInfo, sizeof(memoryInfo)))
		return false;

	meminfo.isCommitted = (memoryInfo.State == MEM_COMMIT);
	meminfo.allocationBase = memoryInfo.BaseAddress;
	meminfo.allocationSize = memoryInfo.RegionSize;
	meminfo.allocationEnd = (MemoryAddress)((size_t)meminfo.allocationBase + meminfo.allocationSize);

	meminfo.isExecutable = WIN32_IS_EXECUTABLE_PROT(memoryInfo.Protect);
	meminfo.isWriteable = WIN32_IS_WRITEABLE_PROT(memoryInfo.Protect);
	return true;
}

bool ScannerTargetWindows::getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const
{
	start = mainModuleStart;
	end = mainModuleEnd;
	return true;
}

bool ScannerTargetWindows::read(const MemoryAddress &adr, const size_t objectSize, void* result) const
{
	ASSERT(this->isAttached());
	return (ReadProcessMemory(this->processHandle, adr, result, objectSize, NULL) != 0);
}

MemoryAddress ScannerTargetWindows::getMainModuleBaseAddress() const
{
	// get the address of kernel32.dll
	HMODULE k32 = GetModuleHandleA("kernel32.dll");

	// get the address of GetModuleHandle()
	LPVOID funcAdr = GetProcAddress(k32, "GetModuleHandleA");
	if (!funcAdr)
		funcAdr = GetProcAddress(k32, "GetModuleHandleW");

	// create the thread
	HANDLE thread = CreateRemoteThread(this->processHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)funcAdr, NULL, NULL, NULL);

	// let the thread finish
	WaitForSingleObject(thread, INFINITE);

	// get the exit code
	MemoryAddress baseAddress;
	GetExitCodeThread(thread, (LPDWORD)&baseAddress);

	// clean up the thread handle
	CloseHandle(thread);

	return baseAddress;
}