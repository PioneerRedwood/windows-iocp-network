#pragma once
#include "predef.hpp"
#include "Exception.hpp"
#include "ClientSession.hpp"
#include "SessionManager.hpp"
#include "IocpManager.hpp"

constexpr auto LISTEN_PORT = 9001;

__declspec(thread) int LThreadType = -1;

enum class THREAD_TYPE : int
{
	THREAD_MAIN_ACCEPT,
	THREAD_IO_WORKER,
};

extern __declspec(thread) int LThreadType;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = static_cast<int>(THREAD_TYPE::THREAD_MAIN_ACCEPT);

	SetUnhandledExceptionFilter(ExceptionFilter);

	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;

	if(false == GIocpManager->Initialize())
	{
		return -1;
	}

	if (false == GIocpManager->StartIoThreads())
	{
		return -1;
	}

	std::cout << "Start Server\n";
	if (false == GIocpManager->StartAccept())
	{
		return -1;
	}

	GIocpManager->Finalize();

	std::cout << "End Server\n";

	delete GIocpManager;
	delete GSessionManager;
}