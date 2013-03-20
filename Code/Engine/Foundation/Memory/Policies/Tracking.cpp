#include <Foundation/PCH.h>
#include <Foundation/Memory/Policies/Tracking.h>

/// \todo Clemens: implement stack tracking once we have a hashtable collection
#if 0
#include "Utils/StackTracer.h"

enum { INITIAL_TRACE_TABLE_SIZE = 128 };

StackTracking::StackTracking() :
  m_trackings(INITIAL_TRACE_TABLE_SIZE, DebugAllocator)
{
}

void StackTracking::AddAllocation(void* ptr, size_t allocatedSize, size_t usedMemorySize,
  const SourceLocation& location)
{
  ezSimpleTracking::AddAllocation(ptr, allocatedSize, usedMemorySize, location);

  // get stacktrace and skip the last three frames since they are useless
  ulong buffer[64];
  int numTraces = utils::StackTracer::GetStackTrace(buffer, 64) - 3;

  ulong* trace = DebugAllocator->CreateBuffer<ulong>(numTraces);
  memory::Utils::Copy(trace, buffer, numTraces);

  TrackingInfo info = { allocatedSize, trace };
  m_trackings.Add(ptr, info);
}

void StackTracking::RemoveAllocation(void* ptr, size_t allocatedSize, size_t usedMemorySize)
{
  ezSimpleTracking::RemoveAllocation(ptr, allocatedSize, usedMemorySize);

  TrackingInfo info = { 0, NULL };
  bool result = m_trackings.Remove(ptr, info);
  __ASSERT(result);

  DebugAllocator->DeleteBuffer(info.trace);
}

void StackTracking::DumpMemoryLeaks()
{
  wchar_t buffer[512];

  for (HashTable<void*, TrackingInfo>::Iterator i = m_trackings.Iterate(); i.Next();)
  {
    void* ptr = i.Key();
    TrackingInfo info = i.Value();
    int numTraces = (int)(DebugAllocator->AllocatedSize(info.trace) / sizeof(ulong));

    swprintf_s(buffer, L"Leaked %d bytes allocated from:\n", (int)info.allocatedSize);
    OutputDebugStringW(buffer);
    utils::StackTracer::DumpStackTrace(info.trace, numTraces);
    OutputDebugStringW(
      L"--------------------------------------------------------------------\n\n");

    DebugAllocator->DeleteBuffer(info.trace);
  }
  m_trackings.Clear();
}
#endif
