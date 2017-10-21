#include "ScannerTargetWindowsStandard.h"
#include <Windows.h>


ScannerTargetWindowsStandard::ScannerTargetWindowsStandard() :
	ScannerTargetWindowsBase() { }

ScannerTargetWindowsStandard::~ScannerTargetWindowsStandard() { }

ProcessHandle ScannerTargetWindowsStandard::obtainProcessHandle(const ProcessIdentifier &pid) const
{
	DWORD _pid = static_cast<DWORD>(pid);
	auto handle = OpenProcess(
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD,
		FALSE,
		_pid
	);
	return handle;
}