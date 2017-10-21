#include "SystemHandle.h"
#include <iostream>

bool SystemHandle::NtFunctionsLoaded = false;
_NtDuplicateObject SystemHandle::NtDuplicateObject = nullptr;
_NtClose SystemHandle::NtClose = nullptr;
_NtQuerySystemInformation SystemHandle::NtQuerySystemInformation = nullptr;
_NtQueryObject SystemHandle::NtQueryObject = nullptr;

SystemHandle::SystemHandle(HANDLE process, SYSTEM_HANDLE_EX handle)
{
	loadNtFunctions();

	this->systemHandle = handle;
	this->process = process;
	this->handleName = L"<unknown>";
	this->initializeHandleDetails();
}
SystemHandle::~SystemHandle()
{
}

const SYSTEM_HANDLE_EX SystemHandle::getNativeSystemHandle() const
{
	return this->systemHandle;
}
const HANDLE SystemHandle::getNativeHandle() const
{
	return (HANDLE)this->systemHandle.dwHandleValue;
}
const std::wstring SystemHandle::getName() const
{
	return this->handleName;
}
const std::wstring SystemHandle::getTypeName() const
{
	return this->handleTypeName;
}
ACCESS_MASK SystemHandle::getGrantedAccess() const
{
	return this->systemHandle.GrantedAccess;
}

bool SystemHandle::isProcessHandle() const
{
	return this->getTypeName().find(L"Process") != std::wstring::npos;
}

HANDLE SystemHandle::duplicateNativeHandle(ACCESS_MASK accessRights) const
{
	HANDLE hFake;
	if (NT_SUCCESS(NtDuplicateObject(
					this->process,
					this->getNativeHandle(),
					GetCurrentProcess(),
					&hFake,
					accessRights,
					0,
					(accessRights == 0) ? DUPLICATE_SAME_ACCESS : 0
	)))
		return hFake;
	return INVALID_HANDLE_VALUE;
}

void SystemHandle::initializeHandleDetails()
{
	auto fake = this->duplicateNativeHandle(0);
	if (fake == INVALID_HANDLE_VALUE)
		return;

	// get the handle name
	DWORD read;
	BYTE nameInfo[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
	if (NT_SUCCESS(NtQueryObject(fake, ObjectNameInformation, nameInfo, sizeof(nameInfo), &read)))
	{
		UNICODE_STRING name = *(PUNICODE_STRING)nameInfo;
		if (name.Buffer && name.Length)
			this->handleName = std::wstring(&name.Buffer[0], &name.Buffer[name.Length - 1]);
	}

	// get the handle type information
	BYTE typeInfoBuffer[sizeof(OBJECT_TYPE_INFORMATION) + 1000]; // some extra bytes for the UNICODE_STRING
	auto typeInfo = (POBJECT_TYPE_INFORMATION)&typeInfoBuffer[0];
	if (NT_SUCCESS(NtQueryObject(fake, ObjectTypeInformation, typeInfo, sizeof(typeInfoBuffer), &read)))
	{
		if (typeInfo->Name.Buffer && typeInfo->Name.Length)
			this->handleTypeName = std::wstring(&typeInfo->Name.Buffer[0], &typeInfo->Name.Buffer[typeInfo->Name.Length - 1]);
	}

	CloseHandle(fake);
}

std::vector<SystemHandle> SystemHandle::enumerateProcessHandles(HANDLE process)
{
	loadNtFunctions();

	std::vector<SystemHandle> ret;
	DWORD handleInfoSize = 4096;
	PSYSTEM_HANDLE_INFORMATION_EX handleInfo = nullptr;

	do
	{
		NTSTATUS result = 0;
		do
		{
			if (handleInfo) delete[] handleInfo;
			handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)new char[handleInfoSize];
			result = NtQuerySystemInformation(SystemExtendedHandleInformation, handleInfo, handleInfoSize, &handleInfoSize);
		} while (result == STATUS_INFO_LENGTH_MISMATCH);

		if (!NT_SUCCESS(result))
			break;

		auto pid = GetProcessId(process);
		for (size_t i = 0; i < handleInfo->dwCount; i++)
			if (handleInfo->Handles[i].dwProcessId == pid)
				ret.push_back(SystemHandle(process, handleInfo->Handles[i]));
	} while (0);

	if (handleInfo)
	{
		delete[] handleInfo;
		handleInfo = nullptr;
	}
	return ret;
}

void SystemHandle::loadNtFunctions()
{
	if (NtFunctionsLoaded) return;
	HINSTANCE NTdll = LoadLibraryA("ntdll.dll");
	if (NTdll)
	{
		NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(NTdll, "NtDuplicateObject");
		NtClose = (_NtClose)GetProcAddress(NTdll, "NtClose");
		NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(NTdll, "NtQuerySystemInformation");
		NtQueryObject = (_NtQueryObject)GetProcAddress(NTdll, "NtQueryObject");
		NtFunctionsLoaded = true;
	}
	else
		std::cerr << "Failed to load ntdll functions! This should never happen and is probably fatal" << std::endl;
}