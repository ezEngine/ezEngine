#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

typedef CRITICAL_SECTION ezMutexHandle;
typedef HANDLE ezThreadHandle;
typedef DWORD (__stdcall *ezOSThreadEntryPoint)(LPVOID lpThreadParameter);
typedef DWORD ezThreadLocalStorageKey;

#define EZ_THREAD_CLASS_ENTRY_POINT DWORD __stdcall ezThreadClassEntryPoint(LPVOID lpThreadParameter);


/// \endcond