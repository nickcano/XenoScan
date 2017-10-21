#include "WindowsUtils.h"
#include <tlhelp32.h>


bool GetSeDebugPrivilege()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	ZeroMemory(&tkprivs, sizeof(tkprivs));

	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken))
		return false;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		CloseHandle(hToken);
		return false;
	}

	tkprivs.PrivilegeCount = 1;
	tkprivs.Privileges[0].Luid = luid;
	tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	BOOL bRet = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
	CloseHandle(hToken);

	return (bRet == TRUE);
}


DWORD GetProcessIdByName(const std::wstring& name)
{
	DWORD result = 0;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32FirstW(snapshot, &entry) == TRUE)
	{
		while (Process32NextW(snapshot, &entry) == TRUE)
		{
			std::wstring binaryPath = entry.szExeFile;
			if (binaryPath.length() < name.length())
				continue;
			auto found = binaryPath.rfind(name);
			if (found != std::wstring::npos && found == binaryPath.length() - name.length())
			{
				result = entry.th32ProcessID;
				break;
			}
		}
	}

	CloseHandle(snapshot);
	return result;
}