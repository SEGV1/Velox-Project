// Provides an efficient blocking version of moodycamel::ConcurrentQueue.
// ©2015-2016 Cameron Desrochers. Distributed under the terms of the simplified
// BSD license, available at the top of concurrentqueue.h.
// Uses Jeff Preshing's semaphore implementation (under the terms of its
// separate zlib license, embedded below).

#pragma once

#include "concurrentqueue.h"
#include <type_traits>
#include <cerrno>
#include <memory>
#include <chrono>
#include <ctime>

#if defined(_WIN32)
// Avoid including windows.h in a header; we only need a handful of
// items, so we'll redeclare them here (this is relatively safe since
// the API generally has to remain stable between Windows versions).
// I know this is an ugly hack but it still beats polluting the global
// namespace with thousands of generic names or adding a .cpp for nothing.
// extern "C" {
// 	struct _SECURITY_ATTRIBUTES;
// 	__declspec(dllimport) void* __stdcall CreateSemaphoreW(_SECURITY_ATTRIBUTES* lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, const wchar_t* lpName);
// 	__declspec(dllimport) int __stdcall CloseHandle(void* hObject);
// 	__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void* hHandle, unsigned long dwMilliseconds);
// 	__declspec(dllimport) int __stdcall ReleaseSemaphore(void* hSemaphore, long lReleaseCount, long* lpPreviousCount);
// }
#elif defined(__MACH__)
