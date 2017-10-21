#pragma once

#include <windows.h>
#include <shellapi.h>
#include <vector>

#include "WindowsUtils.h"

class SystemHandle
{
public:
	static bool NtFunctionsLoaded;
	static _NtDuplicateObject NtDuplicateObject;
	static _NtClose NtClose;
	static _NtQuerySystemInformation NtQuerySystemInformation;
	static _NtQueryObject NtQueryObject;

	static std::vector<SystemHandle> enumerateProcessHandles(HANDLE process);

	SystemHandle(HANDLE process, SYSTEM_HANDLE_EX handle);
	SystemHandle::~SystemHandle();


	const SYSTEM_HANDLE_EX getNativeSystemHandle() const;
	const HANDLE getNativeHandle() const;
	HANDLE duplicateNativeHandle(ACCESS_MASK accessRights) const;
	const std::wstring getName() const;
	const std::wstring getTypeName() const;
	ACCESS_MASK getGrantedAccess() const;
	bool isProcessHandle() const;

protected:
	std::wstring handleName, handleTypeName;
	SYSTEM_HANDLE_EX systemHandle;
	HANDLE process;

private:
	void initializeHandleDetails();

	static void loadNtFunctions();
};
