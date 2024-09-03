#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && TRACY_ENABLE && TRACY_ENABLE_MEMORY_TRACKING
#  include <tracy/tracy/Tracy.hpp>

#  define EZ_TRACY_CALLSTACK_DEPTH 16
#  define EZ_TRACY_ALLOC_CS(ptr, size, name) TracyAllocNS(ptr, size, EZ_TRACY_CALLSTACK_DEPTH, name)
#  define EZ_TRACY_FREE_CS(ptr, name) TracyFreeNS(ptr, EZ_TRACY_CALLSTACK_DEPTH, name)
#  define EZ_TRACY_ALLOC(ptr, size, name) TracyAllocN(ptr, size, name)
#  define EZ_TRACY_FREE(ptr, name) TracyFreeN(ptr, name)
#else
#  define EZ_TRACY_ALLOC_CS(ptr, size, name)
#  define EZ_TRACY_FREE_CS(ptr, name)
#  define EZ_TRACY_ALLOC(ptr, size, name)
#  define EZ_TRACY_FREE(ptr, name)
#endif

namespace
{
  // no tracking for the tracker data itself
  using TrackerDataAllocator = ezAllocatorWithPolicy<ezAllocPolicyHeap, ezAllocatorTrackingMode::Nothing>;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    EZ_ALWAYS_INLINE AllocatorData() = default;

    ezHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    ezAllocatorTrackingMode m_TrackingMode;

    ezAllocatorId m_ParentId;

    ezAllocator::Stats m_Stats;

    ezHashTable<const void*, ezMemoryTracker::AllocationInfo, ezHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    EZ_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    EZ_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    ezMutex m_Mutex;

    using AllocatorTable = ezIdTable<ezAllocatorId, AllocatorData, TrackerDataAllocatorWrapper>;
    AllocatorTable m_AllocatorData;
  };

  static TrackerData* s_pTrackerData;
  static bool s_bIsInitialized = false;
  static bool s_bIsInitializing = false;

  static void Initialize()
  {
    if (s_bIsInitialized)
      return;

    EZ_ASSERT_DEV(!s_bIsInitializing, "MemoryTracker initialization entered recursively");
    s_bIsInitializing = true;

    if (s_pTrackerDataAllocator == nullptr)
    {
      alignas(EZ_ALIGNMENT_OF(TrackerDataAllocator)) static ezUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      EZ_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      alignas(EZ_ALIGNMENT_OF(TrackerData)) static ezUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      EZ_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized = true;
    s_bIsInitializing = false;
  }

  static void DumpLeak(const ezMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char szBuffer[512];
    ezUInt64 uiSize = info.m_uiSize;
    ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);

    ezLog::Print(szBuffer);

    if (info.GetStackTrace().GetPtr() != nullptr)
    {
      ezStackTracer::ResolveStackTrace(info.GetStackTrace(), &ezLog::Print);
    }

    ezLog::Print("--------------------------------------------------------------------\n\n");
  }
} // namespace

// Iterator
#define CAST_ITER(ptr) static_cast<TrackerData::AllocatorTable::Iterator*>(ptr)

ezAllocatorId ezMemoryTracker::Iterator::Id() const
{
  return CAST_ITER(m_pData)->Id();
}

ezStringView ezMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName;
}

ezAllocatorId ezMemoryTracker::Iterator::ParentId() const
{
  return CAST_ITER(m_pData)->Value().m_ParentId;
}

const ezAllocator::Stats& ezMemoryTracker::Iterator::Stats() const
{
  return CAST_ITER(m_pData)->Value().m_Stats;
}

void ezMemoryTracker::Iterator::Next()
{
  CAST_ITER(m_pData)->Next();
}

bool ezMemoryTracker::Iterator::IsValid() const
{
  return CAST_ITER(m_pData)->IsValid();
}

ezMemoryTracker::Iterator::~Iterator()
{
  auto it = CAST_ITER(m_pData);
  EZ_DELETE(s_pTrackerDataAllocator, it);
  m_pData = nullptr;
}


// static
ezAllocatorId ezMemoryTracker::RegisterAllocator(ezStringView sName, ezAllocatorTrackingMode mode, ezAllocatorId parentId)
{
  Initialize();

  EZ_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = sName;
  data.m_TrackingMode = mode;
  data.m_ParentId = parentId;

  return s_pTrackerData->m_AllocatorData.Insert(data);
}

// static
void ezMemoryTracker::DeregisterAllocator(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  ezUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0 && data.m_TrackingMode > ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }

    EZ_REPORT_FAILURE("Allocator '{0}' leaked {1} allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

// static
void ezMemoryTracker::AddAllocation(ezAllocatorId allocatorId, ezAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, ezTime allocationTime)
{
  EZ_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  ezArrayPtr<void*> stackTrace;
  if (mode >= ezAllocatorTrackingMode::AllocationStatsAndStacktraces)
  {
    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

    stackTrace = EZ_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces);
    ezMemoryUtils::Copy(stackTrace.GetPtr(), pBuffer, uiNumTraces);
  }

  {
    EZ_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
    data.m_Stats.m_uiNumAllocations++;
    data.m_Stats.m_uiAllocationSize += uiSize;
    data.m_Stats.m_uiPerFrameAllocationSize += uiSize;
    data.m_Stats.m_PerFrameAllocationTime += allocationTime;

    auto pInfo = &data.m_Allocations[pPtr];
    pInfo->m_uiSize = uiSize;
    pInfo->m_uiAlignment = (ezUInt16)uiAlign;
    pInfo->SetStackTrace(stackTrace);

    if (mode >= ezAllocatorTrackingMode::AllocationStatsAndStacktraces)
    {
      EZ_TRACY_ALLOC_CS(pPtr, uiSize, data.m_sName.GetData());
    }
    else
    {
      EZ_TRACY_ALLOC(pPtr, uiSize, data.m_sName.GetData());
    }
  }
}

// static
void ezMemoryTracker::RemoveAllocation(ezAllocatorId allocatorId, const void* pPtr)
{
  ezArrayPtr<void*> stackTrace;

  {
    EZ_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

    AllocationInfo info;
    if (data.m_Allocations.Remove(pPtr, &info))
    {
      data.m_Stats.m_uiNumDeallocations++;
      data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

      stackTrace = info.GetStackTrace();

      if (data.m_TrackingMode >= ezAllocatorTrackingMode::AllocationStatsAndStacktraces)
      {
        EZ_TRACY_FREE_CS(pPtr, data.m_sName.GetData());
      }
      else
      {
        EZ_TRACY_FREE(pPtr, data.m_sName.GetData());
      }
    }
    else
    {
      EZ_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", ezArgP(pPtr));
    }
  }

  EZ_DELETE_ARRAY(s_pTrackerDataAllocator, stackTrace);
}

// static
void ezMemoryTracker::RemoveAllAllocations(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

    if (data.m_TrackingMode >= ezAllocatorTrackingMode::AllocationStatsAndStacktraces)
    {
      for (const auto& alloc : data.m_Allocations)
      {
        EZ_IGNORE_UNUSED(alloc);
        EZ_TRACY_FREE_CS(alloc.Key(), data.m_sName.GetData());
      }
    }
    else
    {
      for (const auto& alloc : data.m_Allocations)
      {
        EZ_IGNORE_UNUSED(alloc);
        EZ_TRACY_FREE(alloc.Key(), data.m_sName.GetData());
      }
    }

    EZ_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void ezMemoryTracker::SetAllocatorStats(ezAllocatorId allocatorId, const ezAllocator::Stats& stats)
{
  EZ_LOCK(*s_pTrackerData);

  s_pTrackerData->m_AllocatorData[allocatorId].m_Stats = stats;
}

// static
void ezMemoryTracker::ResetPerFrameAllocatorStats()
{
  EZ_LOCK(*s_pTrackerData);

  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    AllocatorData& data = it.Value();
    data.m_Stats.m_uiPerFrameAllocationSize = 0;
    data.m_Stats.m_PerFrameAllocationTime = ezTime::MakeZero();
  }
}

// static
ezStringView ezMemoryTracker::GetAllocatorName(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName;
}

// static
const ezAllocator::Stats& ezMemoryTracker::GetAllocatorStats(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

// static
ezAllocatorId ezMemoryTracker::GetAllocatorParentId(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_ParentId;
}

// static
const ezMemoryTracker::AllocationInfo& ezMemoryTracker::GetAllocationInfo(ezAllocatorId allocatorId, const void* pPtr)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(pPtr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  EZ_REPORT_FAILURE("Could not find info for allocation {0}", ezArgP(pPtr));
  return invalidInfo;
}

struct LeakInfo
{
  EZ_DECLARE_POD_TYPE();

  ezAllocatorId m_AllocatorId;
  size_t m_uiSize = 0;
  bool m_bIsRootLeak = true;
};

// static
ezUInt32 ezMemoryTracker::PrintMemoryLeaks(PrintFunc printfunc)
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return 0;

  EZ_LOCK(*s_pTrackerData);

  ezHashTable<const void*, LeakInfo, ezHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize = it2.Value().m_uiSize;

      if (data.m_TrackingMode == ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        leak.m_bIsRootLeak = false;
      }

      leakTable.Insert(it2.Key(), leak);
    }
  }

  // find dependencies
  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    const void* curPtr = ptr;
    const void* endPtr = ezMemoryUtils::AddByteOffset(ptr, leak.m_uiSize);

    while (curPtr < endPtr)
    {
      const void* testPtr = *reinterpret_cast<const void* const*>(curPtr);

      LeakInfo* dependentLeak = nullptr;
      if (leakTable.TryGetValue(testPtr, dependentLeak))
      {
        dependentLeak->m_bIsRootLeak = false;
      }

      curPtr = ezMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  ezUInt32 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.m_bIsRootLeak)
    {
      const AllocatorData& data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];

      if (data.m_TrackingMode != ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        if (uiNumLeaks == 0)
        {
          printfunc("\n\n--------------------------------------------------------------------\n"
                    "Memory Leak Report:"
                    "\n--------------------------------------------------------------------\n\n");
        }

        ezMemoryTracker::AllocationInfo info;
        data.m_Allocations.TryGetValue(ptr, info);

        DumpLeak(info, data.m_sName.GetData());

        ++uiNumLeaks;
      }
    }
  }

  if (uiNumLeaks > 0)
  {
    char tmp[1024];
    ezStringUtils::snprintf(tmp, 1024, "\n--------------------------------------------------------------------\n"
                                       "Found %u root memory leak(s)."
                                       "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);

    printfunc(tmp);
  }

  return uiNumLeaks;
}

// static
void ezMemoryTracker::DumpMemoryLeaks()
{
  const ezUInt32 uiNumLeaks = PrintMemoryLeaks(ezLog::Print);

  if (uiNumLeaks > 0)
  {
    EZ_REPORT_FAILURE("Found {0} root memory leak(s). See console output for details.", uiNumLeaks);
  }
}

// static
ezMemoryTracker::Iterator ezMemoryTracker::GetIterator()
{
  auto pInnerIt = EZ_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}


