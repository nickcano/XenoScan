#include "ScannerTargetWindowsDuplicate.h"
#include "SystemHandle.h"
#include "WindowsUtils.h"

ScannerTargetWindowsDuplicate::ScannerTargetWindowsDuplicate() :
	ScannerTargetWindowsBase() { }

ScannerTargetWindowsDuplicate::~ScannerTargetWindowsDuplicate() { }

ProcessHandle ScannerTargetWindowsDuplicate::obtainProcessHandle(const ProcessIdentifier &pid) const
{
	HANDLE ret = INVALID_HANDLE_VALUE;
	HANDLE lsassHandle = INVALID_HANDLE_VALUE;
	DWORD _pid = static_cast<DWORD>(pid);
	ACCESS_MASK requiredAccess = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD;

	do
	{
		if (!GetSeDebugPrivilege())
			break;

		auto lsasspid = GetProcessIdByName(L"lsass.exe");
		if (!lsasspid)
			break;

		lsassHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, FALSE, lsasspid);
		if (!lsassHandle || lsassHandle == INVALID_HANDLE_VALUE)
			break;

		auto handles = SystemHandle::enumerateProcessHandles(lsassHandle);

		// first, we'll make a list of the handles which we may be able to duplicate
		std::vector<SystemHandle> candidates;
		for (auto i = handles.begin(); i != handles.end(); i++)
		{
			if (i->isProcessHandle())
			{
				auto handle = i->duplicateNativeHandle(0);
				if (handle == INVALID_HANDLE_VALUE)
					continue;

				auto dupepid = GetProcessId(handle);
				if (dupepid == _pid)
					candidates.push_back(*i);
				CloseHandle(handle);
			}
		}

		// now, we'll see if we're lucky enough for one to already have the access rights we need
		// and, if so, grab it and return
		for (auto i = candidates.begin(); i != candidates.end(); i++)
		{
			if ((i->getGrantedAccess() & requiredAccess) != requiredAccess)
				continue;
			auto handle = i->duplicateNativeHandle(0);
			if (handle == INVALID_HANDLE_VALUE)
				continue;
			ret = handle;
			break;
		}
		if (ret != INVALID_HANDLE_VALUE)
			break;

		// if we weren't so lucky, we can try to duplicate it with the proper acess rights (more likely to fail)
		for (auto i = candidates.begin(); i != candidates.end(); i++)
		{
			auto handle = i->duplicateNativeHandle(requiredAccess);
			if (handle == INVALID_HANDLE_VALUE)
				continue;
			ret = handle;
			break;
		}
		if (ret != INVALID_HANDLE_VALUE)
			break;

		// if we've still failed, we can attempt to fall back to the regular method. why don't we
		// do this to begin with, you ask? well, some anti-cheat software may actually allow
		// this call to succeed, but strip off some of the access rights we need, causing
		// us to think it succeeded but fail down the line when we try to do something.
		// however, it may still work if there's no anti-cheat and someone is using this
		// method for no reason. so we might as well have it as the last option rather than
		// the first.
		ret = OpenProcess(requiredAccess, FALSE, _pid);
	} while (0);

	if (lsassHandle != INVALID_HANDLE_VALUE)
		CloseHandle(lsassHandle);
	return ret;
}