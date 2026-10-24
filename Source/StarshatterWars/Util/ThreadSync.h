/*  Project Starshatter Wars
    Fractal Dev Games
    Copyright (C) 2024. All Rights Reserved.

    SUBSYSTEM:    Foundation
    FILE:         ThreadSync.h
    AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#include "Windows/WindowsHWrapper.h"
#ifdef GetObject
#undef GetObject
#endif

class STARSHATTERWARS_API ThreadSync
{
#if defined(_MT)
    CRITICAL_SECTION sync;
public:
    ThreadSync() { ::InitializeCriticalSection(&sync); }
    ~ThreadSync() { ::DeleteCriticalSection(&sync); }
    void acquire() { ::EnterCriticalSection(&sync); }
    void release() { ::LeaveCriticalSection(&sync); }
#else
public:
    ThreadSync() { }
    ~ThreadSync() { }
    void acquire() { }
    void release() { }
#endif
};

class AutoThreadSync
{
public:
    AutoThreadSync(ThreadSync& s) : sync(s) { sync.acquire(); }
    ~AutoThreadSync() { sync.release(); }
private:
    ThreadSync& sync;
};
