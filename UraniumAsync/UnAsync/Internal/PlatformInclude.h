#pragma once
#include <UnTL/Base/Platform.h>

#if UN_WINDOWS
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    include <atlbase.h>
#    include <atlcom.h>
#    include <guiddef.h>

#    undef CopyMemory
#    undef GetObject
#    undef CreateWindow
#    undef MemoryBarrier
#else
#    include <linux/futex.h>
#    include <sys/syscall.h>
#    include <sys/time.h>
#    include <unistd.h>
#endif
