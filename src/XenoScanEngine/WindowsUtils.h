#pragma once

#include <windows.h>
#include <string>

#pragma warning(disable:4005)
#include <ntstatus.h>
#pragma warning(default:4005)

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

typedef ULONG(NTAPI *_NtDuplicateObject)(
	HANDLE SourceProcessHandle,
	HANDLE SourceHandle,
	HANDLE TargetProcessHandle,
	PHANDLE TargetHandle,
	ACCESS_MASK DesiredAccess,
	ULONG Attributes,
	ULONG Options
	);

typedef NTSTATUS(NTAPI *_NtClose)(
	HANDLE Handle
	);

typedef NTSTATUS(NTAPI *_NtQuerySystemInformation)(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

typedef NTSTATUS(NTAPI *_NtQueryObject)(
	HANDLE Handle,
	ULONG ObjectInformationClass,
	PVOID ObjectInformation,
	ULONG ObjectInformationLength,
	PULONG ReturnLength

	);

typedef struct _HANDLE_INFO
{
	USHORT tcDeviceName[260];
	USHORT tcFileName[260];
	ULONG uType;
}HANDLE_INFO;

typedef struct _ADDRESS_INFO
{
	PVOID pAddress;
}ADDRESS_INFO;

typedef LONG NTSTATUS;

typedef struct _SYSTEM_HANDLE
{
	DWORD       dwProcessId;
	BYTE		bObjectType;
	BYTE		bFlags;
	WORD		wValue;
	PVOID       pAddress;
	DWORD GrantedAccess;
}SYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	DWORD         dwCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION, **PPSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_HANDLE_EX {
	PVOID Object;
	DWORD dwProcessId;
	DWORD dwHandleValue;
	ULONG GrantedAccess;
	USHORT CreatorBackTraceIndex;
	USHORT ObjectTypeIndex;
	ULONG  HandleAttributes;
	ULONG  Reserved;
} SYSTEM_HANDLE_EX, *PSYSTEM_HANDLE_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
	ULONG_PTR dwCount;
	ULONG_PTR Reserved;
	SYSTEM_HANDLE_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;


typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemHandleInformation = 0x10,
	SystemExtendedHandleInformation = 0x40,
} SYSTEM_INFORMATION_CLASS;

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation = 0x0,
	ObjectNameInformation = 0x1,
	ObjectTypeInformation = 0x2
} _OBJECT_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
	USHORT  Length;
	USHORT  MaximumLength;
	PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef enum _POOL_TYPE
{
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING  Name;
	ULONG           TotalNumberOfObjects;
	ULONG           TotalNumberOfHandles;
	ULONG           TotalPagedPoolUsage;
	ULONG           TotalNonPagedPoolUsage;
	ULONG           TotalNamePoolUsage;
	ULONG           TotalHandleTableUsage;
	ULONG           HighWaterNumberOfObjects;
	ULONG           HighWaterNumberOfHandles;
	ULONG           HighWaterPagedPoolUsage;
	ULONG           HighWaterNonPagedPoolUsage;
	ULONG           HighWaterNamePoolUsage;
	ULONG           HighWaterHandleTableUsage;
	ULONG           InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG           ValidAccess;
	BOOLEAN         SecurityRequired;
	BOOLEAN         MaintainHandleCount;
	USHORT          MaintainTypeList;
	POOL_TYPE       PoolType;
	ULONG           PagedPoolUsage;
	ULONG           NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

#define IOCTL_LISTDRV_BUFFERED_IO		\
        CTL_CODE(FILE_DEVICE_UNKNOWN,	\
        0x802,							\
        METHOD_BUFFERED,	            \
        FILE_READ_DATA | FILE_WRITE_DATA)


typedef NTSTATUS(WINAPI *PNtQuerySystemInformation)
(IN	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT	PVOID					 SystemInformation,
	IN	ULONG					 SystemInformationLength,
	OUT	PULONG					 ReturnLength OPTIONAL);


bool GetSeDebugPrivilege();
DWORD GetProcessIdByName(const std::wstring& name);