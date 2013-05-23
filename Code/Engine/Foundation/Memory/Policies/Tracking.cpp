#include <Foundation/PCH.h>
#include <Foundation/Memory/Policies/Tracking.h>
#include <Foundation/Utilities/StackTracer.h>

namespace ezMemoryPolicies
{

enum { INITIAL_TRACE_TABLE_SIZE = 128 };

ezStackTracking::ezStackTracking() :
  m_trackings(ezFoundation::GetDebugAllocator())
{
  m_trackings.Reserve(INITIAL_TRACE_TABLE_SIZE);
}

ezStackTracking::~ezStackTracking()
{
  for (ezHashTable<void*, TrackingInfo>::ConstIterator it = m_trackings.GetIterator(); it.IsValid(); ++it)
  {
    TrackingInfo info = it.Value();
    EZ_DELETE_RAW_BUFFER(ezFoundation::GetDebugAllocator(), info.pTrace);
  }

  m_trackings.Clear();
}

void ezStackTracking::AddAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize)
{
  ezSimpleTracking::AddAllocation(ptr, uiAllocatedSize, uiUsedMemorySize);

  void* pBuffer[64];
  ezArrayPtr<void*> tempTrace(pBuffer);
  const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

  void** pTrace = EZ_NEW_RAW_BUFFER(ezFoundation::GetDebugAllocator(), void*, uiNumTraces);
  ezMemoryUtils::Copy(pTrace, tempTrace.GetPtr(), uiNumTraces);

  TrackingInfo info = { uiAllocatedSize, pTrace };
  m_trackings.Insert(ptr, info);
}

void ezStackTracking::RemoveAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize)
{
  ezSimpleTracking::RemoveAllocation(ptr, uiAllocatedSize, uiUsedMemorySize);

  TrackingInfo info = { 0, NULL };
  bool result = m_trackings.Remove(ptr, &info);
  EZ_ASSERT(result, "No tracking information found for '%x'", ptr);

  EZ_DELETE_RAW_BUFFER(ezFoundation::GetDebugAllocator(), info.pTrace);
}

void ezStackTracking::DumpMemoryLeaks() const
{
  for (ezHashTable<void*, TrackingInfo>::ConstIterator it = m_trackings.GetIterator(); it.IsValid(); ++it)
  {
    TrackingInfo info = it.Value();
    const ezUInt32 uiNumTraces = (ezUInt32)(ezFoundation::GetDebugAllocator()->AllocatedSize(info.pTrace) / sizeof(void*));
    
    EZ_IGNORE_UNUSED(uiNumTraces);

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    
    wchar_t buffer[512];

    // todo: make this platform independent
    swprintf_s(buffer, L"Leaked %d bytes allocated from:\n", (int)info.uiAllocatedSize);
    OutputDebugStringW(buffer);
    ezStackTracer::DumpStackTrace(ezArrayPtr<void*>(info.pTrace, uiNumTraces));
    OutputDebugStringW(
      L"--------------------------------------------------------------------\n\n");
  #endif

  }
}

}
