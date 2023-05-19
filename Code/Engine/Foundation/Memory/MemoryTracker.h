#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>

struct ezMemoryTrackingFlags
{
  using StorageType = ezUInt32;

  enum Enum
  {
    None,
    RegisterAllocator = EZ_BIT(0),        ///< Register the allocator with the memory tracker. If EnableAllocationTracking is not set as well it is up to the
                                          ///< allocator implementation whether it collects usable stats or not.
    EnableAllocationTracking = EZ_BIT(1), ///< Enable tracking of individual allocations
    EnableStackTrace = EZ_BIT(2),         ///< Enable stack traces for each allocation

    All = RegisterAllocator | EnableAllocationTracking | EnableStackTrace,

    Default = 0
#if EZ_ENABLED(EZ_USE_ALLOCATION_TRACKING)
              | RegisterAllocator | EnableAllocationTracking
#endif
#if EZ_ENABLED(EZ_USE_ALLOCATION_STACK_TRACING)
              | EnableStackTrace
#endif
  };

  struct Bits
  {
    StorageType RegisterAllocator : 1;
    StorageType EnableAllocationTracking : 1;
    StorageType EnableStackTrace : 1;
  };
};

// EZ_DECLARE_FLAGS_OPERATORS(ezMemoryTrackingFlags);

#define EZ_STATIC_ALLOCATOR_NAME "Statics"

/// \brief Memory tracker which keeps track of all allocations and constructions
class EZ_FOUNDATION_DLL ezMemoryTracker
{
public:
  struct AllocationInfo
  {
    EZ_DECLARE_POD_TYPE();

    EZ_FORCE_INLINE AllocationInfo()
       
    {
    }

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
    const char* Name() const;
    ezAllocatorId ParentId() const;
    const ezAllocatorBase::Stats& Stats() const;

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

  static ezAllocatorId RegisterAllocator(const char* szName, ezBitflags<ezMemoryTrackingFlags> flags, ezAllocatorId parentId);
  static void DeregisterAllocator(ezAllocatorId allocatorId);

  static void AddAllocation(
    ezAllocatorId allocatorId, ezBitflags<ezMemoryTrackingFlags> flags, const void* pPtr, size_t uiSize, size_t uiAlign, ezTime allocationTime);
  static void RemoveAllocation(ezAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(ezAllocatorId allocatorId);
  static void SetAllocatorStats(ezAllocatorId allocatorId, const ezAllocatorBase::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static const char* GetAllocatorName(ezAllocatorId allocatorId);
  static const ezAllocatorBase::Stats& GetAllocatorStats(ezAllocatorId allocatorId);
  static ezAllocatorId GetAllocatorParentId(ezAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(ezAllocatorId allocatorId, const void* pPtr);

  static void DumpMemoryLeaks();

  static Iterator GetIterator();
};
