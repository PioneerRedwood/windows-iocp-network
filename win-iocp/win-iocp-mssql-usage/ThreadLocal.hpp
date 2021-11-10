#pragma once
#include <cstdint>

constexpr auto MAX_IO_THREAD = 4;

enum class THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER,
};

class Timer;
class LockOrderChecker;

extern __declspec(thread) int LThreadType;
extern __declspec(thread) int LIoThreadId;
extern __declspec(thread) Timer* LTimer;
extern __declspec(thread) int64_t LTickCount;
extern __declspec(thread) LockOrderChecker LLockOrderChecker;

// cpp
//#include "stdafx.h"
//#include "ThreadLocal.h"
//#include "Timer.h"
//#include "LockOrderChecker.h"
//
//__declspec(thread) int LThreadType = -1;
//__declspec(thread) int LIoThreadId = -1;
//__declspec(thread) Timer* LTimer = nullptr;
//__declspec(thread) int64_t LTickCount = 0;
//__declspec(thread) LockOrderChecker* LLockOrderChecker = nullptr;
