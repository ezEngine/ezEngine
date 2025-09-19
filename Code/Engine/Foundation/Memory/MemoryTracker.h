#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Bitflags.h>

enum class ezAllocatorTrackingMode : ezUInt32
{
  Nothing,                       ///< The allocator doesn't track anything. Use this for best performance.
  Basics,                        ///< The allocator will be known to the system, so it can show up in debugging tools, but barely anything more.
  AllocationStats,               ///< The allocator keeps track of how many allocations and deallocations it did and how large its memory usage is.
  AllocationStatsIgnoreLeaks,    ///< Same as AllocationStats, but any remaining allocations at shutdown are not reported as leaks.
  AllocationStatsAndStacktraces, ///< The allocator will record stack traces for each allocation, which can be used to find memory leaks.

  Default = EZ_ALLOC_TRACKING_DEFAULT,
};

/// \brief Global memory tracking system for debugging, profiling, and leak detection.
///
/// This singleton provides comprehensive memory allocation tracking across all allocators
/// in the system. It supports different tracking modes ranging from basic statistics to
/// full stack trace recording for every allocation.
class EZ_FOUNDATION_DLL ezMemoryTracker
{
public:
  struct AllocationInfo
  {
    EZ_DECLARE_POD_TYPE();

    EZ_FORCE_INLINE AllocationInfo()

      = default;

    void** m_pStackTrace = nullptr;
    size_t m_uiSize = 0;
    ezUInt16 m_uiAlignment = 0;
    ezUInt16 m_uiStackTraceLength = 0;

    EZ_ALWAYS_INLINE const ezArrayPtr<void*> GetStackTrace() const { return ezArrayPtr<void*>(m_pStackTrace, (ezUInt32)m_uiStackTraceLength); }

    EZ_ALWAYS_INLINE ezArrayPtr<void*> GetStackTrace() { return ezArrayPtr<void*>(m_pStackTrace, (ezUInt32)m_uiStackTraceLength); }

    EZ_FORCE_INLINE void SetStackTrace(ezArrayPtr<void*> stackTrace)
    {
      m_pStackTrace = stackTrace.GetPtr();
      EZ_ASSERT_DEV(stackTrace.GetCount() < 0xFFFF, "stack trace too long");
      m_uiStackTraceLength = (ezUInt16)stackTrace.GetCount();
    }
  };

  class EZ_FOUNDATION_DLL Iterator
  {
  public:
    ~Iterator();

    ezAllocatorId Id() const;
    ezStringView Name() const;
    ezAllocatorId ParentId() const;
    const ezAllocator::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    EZ_ALWAYS_INLINE void operator++() { Next(); }

  private:
    friend class ezMemoryTracker;

    EZ_ALWAYS_INLINE Iterator(void* pData)
      : m_pData(pData)
    {
    }

    void* m_pData;
  };

  static ezAllocatorId RegisterAllocator(ezStringView sName, ezAllocatorTrackingMode mode, ezAllocatorId parentId);
  static void DeregisterAllocator(ezAllocatorId allocatorId);

  static void AddAllocation(ezAllocatorId allocatorId, ezAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, ezTime allocationTime);
  static void RemoveAllocation(ezAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(ezAllocatorId allocatorId);
  static void SetAllocatorStats(ezAllocatorId allocatorId, const ezAllocator::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static ezStringView GetAllocatorName(ezAllocatorId allocatorId);
  static const ezAllocator::Stats& GetAllocatorStats(ezAllocatorId allocatorId);
  static ezAllocatorId GetAllocatorParentId(ezAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(ezAllocatorId allocatorId, const void* pPtr);

  static Iterator GetIterator();

  /// \brief Callback for printing strings.
  using PrintFunc = void (*)(const char* szLine);

  /// \brief Reports back information about all currently known root memory leaks.
  ///
  /// Returns the number of found memory leaks.
  static ezUInt32 PrintMemoryLeaks(PrintFunc printfunc);

  /// \brief Prints the known memory leaks to ezLog and triggers an assert if there are any.
  ///
  /// This is useful to call at the end of an application, to get a debug breakpoint in case of memory leaks.
  static void DumpMemoryLeaks();
};
