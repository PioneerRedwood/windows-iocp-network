#pragma once
#include "predef.hpp"
#include <DbgHelp.h>

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	if (IsDebuggerPresent())
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// dump file �����
	HANDLE hFile = CreateFileA("EasyServer_minidump.dmp", GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = exceptionInfo;
		mdei.ClientPointers = FALSE;

		// enum uint32_t ũ���� 0x00000000 - 0xFFFFFFFF
		// ������ ������ OR �����ڷ� ������
		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
			MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo |
			MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, mdt, (exceptionInfo != 0) ? &mdei : 0, 0, NULL);

		CloseHandle(hFile);
	}
	else
	{
		std::cout << "CreateFile failed. Error: " << GetLastError() << "\n";
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

inline void CRASH_ASSERT(bool isOk)
{
	if (isOk)
	{
		return;
	}

	int* crashVal = 0;
	*crashVal = 0xDEADBEEF;
}
