#include "ScannerTargetDolphin.h"

#include "Assert.h"

ScannerTargetDolphin::ScannerTargetDolphin() :
	sharedMemoryHandle(nullptr)
{
	this->supportedBlueprints.clear();

	this->pointerSize = sizeof(uint16_t);
	this->chunkSize = 0x800000;
	this->littleEndian = false;

	this->detach();

	static_assert(sizeof(uint16_t) <= sizeof(MemoryAddress), "MemoryAddress type is too small!");
}

ScannerTargetDolphin::~ScannerTargetDolphin()
{
	this->detach();
}

bool ScannerTargetDolphin::attach(const ProcessIdentifier &pid)
{
	// only one dolphin emulator can be active atm
	if (this->isAttached())
		return true;

	this->sharedMemoryHandle = ScannerTargetDolphin::obtainSHMHandle();
	if (!this->sharedMemoryHandle)
		return false;

	auto ramView = obtainView(this->sharedMemoryHandle, 0, 0x01800000);
	if (!ramView)
	{
		this->detach();
		return false;
	}
	this->views.push_back(MemoryView(0, 0x01800000, ramView));

	// we good!
	return true;
}

bool ScannerTargetDolphin::isAttached() const
{
	return (this->sharedMemoryHandle != nullptr);
}

bool ScannerTargetDolphin::queryMemory(const MemoryAddress &adr, MemoryInformation& meminfo, MemoryAddress &nextAdr) const
{
	ASSERT(this->isAttached());

	auto retView = this->views.cend();

	// first, let's see if this is within a known view
	for (auto view = this->views.cbegin(); view != this->views.cend(); view++)
	{
		if (adr >= view->start && adr < view->end)
		{
			retView = view;
			break;
		}
	}

	// if it's not, let's find which view is next.
	// We assume that views are in ascending order
	if (retView == this->views.cend())
	{
		for (auto view = this->views.cbegin(); view != this->views.cend(); view++)
		{
			if (adr < view->start)
			{
				retView = view;
				break;
			}
		}
	}

	// now let's set up everything and return
	if (retView != this->views.cend())
	{
		meminfo.isCommitted = true;
		meminfo.allocationBase = retView->start;
		meminfo.allocationSize = retView->size;
		meminfo.allocationEnd = retView->end;

		meminfo.isExecutable = false;
		meminfo.isWriteable = true;

		nextAdr = meminfo.allocationEnd;
		return true;
	}
	return false;
}

bool ScannerTargetDolphin::getMainModuleBounds(MemoryAddress &start, MemoryAddress &end) const
{
	start = 0;
	end = 0;
	return false;
}

bool ScannerTargetDolphin::rawRead(const MemoryAddress &adr, const size_t objectSize, void* result) const
{
	ASSERT(this->isAttached());
	
}

bool ScannerTargetDolphin::rawWrite(const MemoryAddress &adr, const size_t objectSize, const void* const data) const
{
	ASSERT(this->isAttached());
	
}

void ScannerTargetDolphin::detach()
{
	for (auto view = this->views.begin(); view != this->views.end(); view++)
		if (view->buffer)
			ScannerTargetDolphin::releaseView(view->buffer);
	this->views.clear();

	if (this->sharedMemoryHandle)
	{
		ScannerTargetDolphin::releaseSHMHandle(this->sharedMemoryHandle);
		this->sharedMemoryHandle = nullptr;
	}
}



#ifdef WIN32
#include <Windows.h>

/*
	RUNDOWN OF WINDOWS DOLPHIN IMPLEMENTATION:
	
	This code is intended to work on Dolphin 5.0. In this version of Dolphin,
	the emulator's memory map is allocated with `CreateFileMapping` using
	`nullptr` as the `lpName` parameter.

	This leaves us almost, but not quite, able to use the segment as a regular
	shared memory segment, which would be nice. I've patched the assembly code in Dolphin
	as follows:
	  B     00000000007C77FF | ADD QWORD PTR SS:[RSP+28],37F694 <-.   // add 0x37F694 to lpName (0x7C7934) to give 0xF46FC8 (the string we'll use)
	  e     00000000007C7808 | XOR RCX,RCX                        |   // restore RCX (was originally 0)
	  g     00000000007C780B | JMP dolphin.7C7891 -------------.  |   // restore execution to the original code
	  i     ....func code...                                   |  |
	  n --> 00000000007C788C | JMP dolphin.7C792F ----------.  |  |   // jump to our first code cave (original code was MOV QWORD PTR SS:[RSP+28], RCX)
	        ....func code... <------------------------------+--'  |
	        00000000007C792F | CALL dolphin.BC7934 <--------'     |   // call 0 basically RIP on the stack
	        00000000007C7934 | POP RCX                            |   // store RIP in RCX
	        00000000007C7935 | MOV QWORD PTR SS:[RSP+28],RCX      |   // move RIP to [rsp+28] (lpName for CreateFileMapping)
	        00000000007C793A | JMP dolphin.7C77FF ----------------'   // jump to our next code cave


	This patch modifies the behavior of Dolphin such that it will pass the wide
	string "Dolphin Direct3D 11 backend" as the memory map name. With it, we can
	call `OpenFileMappingW` to get a map handle directly to the raw memory used
	by the emulator.
	
	This name for the map was chosen arbitrarily; it was already present in the
	binary and follows the rules for memory map names, plus it is unique enough
	that it shouldn't pose a problem.

	This patch is inteded for the Dolphin.exe shipped with Dolphin 5.0. Identifiers:
		MD5:    5e8f0572abd0f8838780308e9c5dcf52
		SHA256: 19d5c382204d7e40a764e116967aec610f502b9be60b9d3b095073827aa93c66

	The simple explanation is that I changed:
		CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD)(size), nullptr);
	To:
		CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD)(size), L"Dolphin Direct3D 11 backend");
*/

void* ScannerTargetDolphin::obtainSHMHandle()
{
	static_assert(sizeof(void*) >= sizeof(HANDLE), "void* should be able to store a handle!");

	return (void*)OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"Dolphin Direct3D 11 backend");
}
void ScannerTargetDolphin::releaseSHMHandle(const void* handle)
{
	CloseHandle((HANDLE)handle);
}

void* ScannerTargetDolphin::obtainView(const void* handle, const MemoryAddress& offset, size_t size)
{
	return MapViewOfFile((HANDLE)handle, FILE_MAP_ALL_ACCESS, 0, (DWORD)offset, size);
}
void ScannerTargetDolphin::releaseView(const void* viewHandle)
{
	UnmapViewOfFile(viewHandle);
}

#else
// need to implement for other systems here
#endif