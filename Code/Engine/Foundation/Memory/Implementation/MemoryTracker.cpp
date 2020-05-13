#include <FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

namespace
{
  // no tracking for the tracker data itself
  typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation, 0> TrackerDataAllocator;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    EZ_ALWAYS_INLINE AllocatorData() {}

    ezHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    ezBitflags<ezMemoryTrackingFlags> m_Flags;

    ezAllocatorId m_ParentId;

    ezAllocatorBase::Stats m_Stats;

    ezHashTable<const void*, ezMemoryTracker::AllocationInfo, ezHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    EZ_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    EZ_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    ezMutex m_Mutex;

    typedef ezIdTable<ezAllocatorId, AllocatorData, TrackerDataAllocatorWrapper> AllocatorTable;
    AllocatorTable m_AllocatorData;

    ezAllocatorId m_StaticAllocatorId;
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
      EZ_ALIGN_VARIABLE(static ezUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)], EZ_ALIGNMENT_OF(TrackerDataAllocator));
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      EZ_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      EZ_ALIGN_VARIABLE(static ezUInt8 TrackerDataBuffer[sizeof(TrackerData)], EZ_ALIGNMENT_OF(TrackerData));
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

const char* ezMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName.GetData();
}

ezAllocatorId ezMemoryTracker::Iterator::ParentId() const
{
  return CAST_ITER(m_pData)->Value().m_ParentId;
}

const ezAllocatorBase::Stats& ezMemoryTracker::Iterator::Stats() const
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
ezAllocatorId ezMemoryTracker::RegisterAllocator(const char* szName, ezBitflags<ezMemoryTrackingFlags> flags, ezAllocatorId parentId)
{
  Initialize();

  EZ_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = szName;
  data.m_Flags = flags;
  data.m_ParentId = parentId;

  ezAllocatorId id = s_pTrackerData->m_AllocatorData.Insert(data);

  if (data.m_sName == EZ_STATIC_ALLOCATOR_NAME)
  {
    s_pTrackerData->m_StaticAllocatorId = id;
  }

  return id;
}

// static
void ezMemoryTracker::DeregisterAllocator(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  ezUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0)
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
void ezMemoryTracker::AddAllocation(ezAllocatorId allocatorId, ezBitflags<ezMemoryTrackingFlags> flags, const void* ptr, size_t uiSize, size_t uiAlign,
  ezTime allocationTime)
{
  EZ_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  ezArrayPtr<void*> stackTrace;
  if (flags.IsSet(ezMemoryTrackingFlags::EnableStackTrace))
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

    EZ_ASSERT_DEBUG(data.m_Flags == flags, "Given flags have to be identical to allocator flags");
    auto pInfo = &data.m_Allocations[ptr];
    pInfo->m_uiSize = uiSize;
    pInfo->m_uiAlignment = (ezUInt16)uiAlign;
    pInfo->SetStackTrace(stackTrace);
  }
}

// static
void ezMemoryTracker::RemoveAllocation(ezAllocatorId allocatorId, const void* ptr)
{
  ezArrayPtr<void*> stackTrace;

  {
    EZ_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

    AllocationInfo info;
    if (data.m_Allocations.Remove(ptr, &info))
    {
      data.m_Stats.m_uiNumDeallocations++;
      data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

      stackTrace = info.GetStackTrace();
    }
    else
    {
      EZ_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", ezArgP(ptr));
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

    EZ_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void ezMemoryTracker::SetAllocatorStats(ezAllocatorId allocatorId, const ezAllocatorBase::Stats& stats)
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
    data.m_Stats.m_PerFrameAllocationTime.SetZero();
  }
}

// static
const char* ezMemoryTracker::GetAllocatorName(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName.GetData();
}

// static
const ezAllocatorBase::Stats& ezMemoryTracker::GetAllocatorStats(ezAllocatorId allocatorId)
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
const ezMemoryTracker::AllocationInfo& ezMemoryTracker::GetAllocationInfo(ezAllocatorId allocatorId, const void* ptr)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(ptr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  EZ_REPORT_FAILURE("Could not find info for allocation {0}", ezArgP(ptr));
  return invalidInfo;
}


struct LeakInfo
{
  EZ_DECLARE_POD_TYPE();

  ezAllocatorId m_AllocatorId;
  size_t m_uiSize = 0;
  const void* m_pParentLeak = nullptr;

  EZ_ALWAYS_INLINE bool IsRootLeak() const { return m_pParentLeak == nullptr && m_AllocatorId != s_pTrackerData->m_StaticAllocatorId; }
};

// static
void ezMemoryTracker::DumpMemoryLeaks()
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return;
  EZ_LOCK(*s_pTrackerData);

  static ezHashTable<const void*, LeakInfo, ezHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;
  leakTable.Clear();

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize = it2.Value().m_uiSize;
      leak.m_pParentLeak = nullptr;

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
        dependentLeak->m_pParentLeak = ptr;
      }

      curPtr = ezMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  ezUInt64 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.IsRootLeak())
    {
      if (uiNumLeaks == 0)
      {
        ezLog::Print("\n\n--------------------------------------------------------------------\n"
                     "Memory Leak Report:"
                     "\n--------------------------------------------------------------------\n\n");
      }

      const AllocatorData& data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];
      ezMemoryTracker::AllocationInfo info;
      data.m_Allocations.TryGetValue(ptr, info);

      DumpLeak(info, data.m_sName.GetData());

      ++uiNumLeaks;
    }
  }

  if (uiNumLeaks > 0)
  {
    ezLog::Printf("\n--------------------------------------------------------------------\n"
                  "Found %llu root memory leak(s)."
                  "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);

    EZ_REPORT_FAILURE("Found {0} root memory leak(s).", uiNumLeaks);
  }
}

// static
ezMemoryTracker::Iterator ezMemoryTracker::GetIterator()
{
  auto pInnerIt = EZ_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_MemoryTracker);
