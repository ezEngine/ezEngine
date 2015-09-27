#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

typedef ezGenericId<24, 8> ezAllocatorId;

struct ezMemoryTrackingFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None,
    EnableTracking   = EZ_BIT(0),
    EnableStackTrace = EZ_BIT(1),

    All = EnableTracking | EnableStackTrace,

    Default = 0
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    | EnableTracking
#endif
#if EZ_ENABLED(EZ_USE_ALLOCATION_STACK_TRACING)
    | EnableStackTrace
#endif
  };

  struct Bits
  {
    StorageType EnableTracking : 1;
    StorageType EnableStackTrace : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezMemoryTrackingFlags);

#define EZ_STATIC_ALLOCATOR_NAME "Statics"

/// \brief Memory tracker which keeps track of all allocations and constructions
class EZ_FOUNDATION_DLL ezMemoryTracker
{
public:
  struct AllocationInfo
  {
    EZ_DECLARE_POD_TYPE();

    EZ_FORCE_INLINE AllocationInfo()
      : m_pStackTrace(nullptr)
      , m_uiSize(0)
      , m_uiAlignment(0)
      , m_uiStackTraceLength(0)
    {
    }

    void** m_pStackTrace;
    ezUInt32 m_uiSize;
    ezUInt16 m_uiAlignment;
    ezUInt16 m_uiStackTraceLength;

    EZ_FORCE_INLINE const ezArrayPtr<void*> GetStackTrace() const
    {
      return ezArrayPtr<void*>(m_pStackTrace, (ezUInt32)m_uiStackTraceLength);
    }

    EZ_FORCE_INLINE ezArrayPtr<void*> GetStackTrace()
    {
      return ezArrayPtr<void*>(m_pStackTrace, (ezUInt32)m_uiStackTraceLength);
    }

    EZ_FORCE_INLINE  void SetStackTrace(ezArrayPtr<void*> stackTrace)
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

    const char* Name() const;
    const ezAllocatorBase::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    EZ_FORCE_INLINE void operator++() { Next(); }

  private:
    friend class ezMemoryTracker;

    EZ_FORCE_INLINE Iterator(void* pData) : m_pData(pData) { }
    
    void* m_pData;
  };

  static ezAllocatorId RegisterAllocator(const char* szName, ezBitflags<ezMemoryTrackingFlags> flags);
  static void DeregisterAllocator(ezAllocatorId allocatorId);

  static void AddAllocation(ezAllocatorId allocatorId, const void* ptr, size_t uiSize, size_t uiAlign);
  static void RemoveAllocation(ezAllocatorId allocatorId, const void* ptr);
  static void RemoveAllAllocations(ezAllocatorId allocatorId);

  static const char* GetAllocatorName(ezAllocatorId allocatorId);
  static const ezAllocatorBase::Stats& GetAllocatorStats(ezAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(ezAllocatorId allocatorId, const void* ptr);

  static void DumpMemoryLeaks();

  static Iterator GetIterator();
};
