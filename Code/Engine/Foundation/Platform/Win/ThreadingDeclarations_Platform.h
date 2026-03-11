#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <Foundation/Platform/Win/Utils/MinWindows.h>

#if EZ_ENABLED(EZ_PLATFORM_32BIT)
struct alignas(4) ezMutexHandle
{
  ezUInt8 data[24];
};
#else
struct alignas(8) ezMutexHandle
{
  ezUInt8 data[40];
};
#endif


#if EZ_ENABLED(EZ_PLATFORM_32BIT)
struct alignas(4) ezConditionVariableHandle
{
  ezUInt8 data[4];
};
#else
struct alignas(8) ezConditionVariableHandle
{
  ezUInt8 data[8];
};
#endif



using ezThreadHandle = ezMinWindows::HANDLE;
using ezThreadID = ezMinWindows::DWORD;
using ezOSThreadEntryPoint = ezMinWindows::DWORD(__stdcall*)(void* lpThreadParameter);
using ezSemaphoreHandle = ezMinWindows::HANDLE;

#define EZ_THREAD_CLASS_ENTRY_POINT ezMinWindows::DWORD __stdcall ezThreadClassEntryPoint(void* lpThreadParameter);

struct ezConditionVariableData
{
  ezConditionVariableHandle m_ConditionVariable;
};

/// \endcond
