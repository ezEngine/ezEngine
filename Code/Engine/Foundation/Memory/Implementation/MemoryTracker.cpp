#include <Foundation/PCH.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/StackTracer.h>

namespace
{
  // no tracking for the tracker data itself
  typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation, 0> TrackerDataAllocator;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    EZ_FORCE_INLINE static ezAllocatorBase* GetAllocator()
    {
      return s_pTrackerDataAllocator;
    }
  };


  struct AllocatorData
  {
    EZ_FORCE_INLINE AllocatorData()
    {
    }

    ezHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    ezBitflags<ezMemoryTrackingFlags> m_Flags;

    ezAllocatorBase::Stats m_Stats;

    ezHashTable<const void*, ezMemoryTracker::AllocationInfo, ezHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    EZ_FORCE_INLINE void Acquire() { m_Mutex.Acquire(); }
    EZ_FORCE_INLINE void Release() { m_Mutex.Release(); }

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
      static ezUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      EZ_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      static ezUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      EZ_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized = true;
    s_bIsInitializing = false;
  }

  static void PrintHelper(const char* szText)
  {
  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    OutputDebugString(szText);
  #else
    printf("%s", szText);
  #endif
  };

  static void DumpLeak(const ezMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char szBuffer[512];
    ezUInt64 uiSize = info.m_uiSize;
    ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);
    
    PrintHelper(szBuffer);

    if (info.GetStackTrace().GetPtr() != nullptr)
    {
      ezStackTracer::ResolveStackTrace(info.GetStackTrace(), &PrintHelper);
    }
      
    PrintHelper("--------------------------------------------------------------------\n\n");
  }
}

// Iterator
#define CAST_ITER(ptr) static_cast<TrackerData::AllocatorTable::Iterator*>(ptr)

const char* ezMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName.GetData();
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


//static
ezAllocatorId ezMemoryTracker::RegisterAllocator(const char* szName, ezBitflags<ezMemoryTrackingFlags> flags)
{
  Initialize();

  EZ_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = szName;
  data.m_Flags = flags;
  
  ezAllocatorId id = s_pTrackerData->m_AllocatorData.Insert(data);

  if (data.m_sName == EZ_STATIC_ALLOCATOR_NAME)
  {
    s_pTrackerData->m_StaticAllocatorId = id;
  }

  return id;
}

//static
void ezMemoryTracker::DeregisterAllocator(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  ezUInt64 uiLiveAllocations = data.m_Stats.m_uiNumAllocations - data.m_Stats.m_uiNumDeallocations;
  if (uiLiveAllocations != 0 || data.m_Stats.m_uiAllocationSize != 0)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }
    
    EZ_REPORT_FAILURE("Allocator '%s' leaked %llu allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

//static
void ezMemoryTracker::AddAllocation(ezAllocatorId allocatorId, const void* ptr, size_t uiSize, size_t uiAlign)
{
  EZ_LOCK(*s_pTrackerData);

  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  data.m_Stats.m_uiNumAllocations++;
  data.m_Stats.m_uiAllocationSize += uiSize;

  AllocationInfo info;
  EZ_ASSERT_DEV(uiSize < 0xFFFFFFFF, "Allocation size too big");
  EZ_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");
  info.m_uiSize = (ezUInt32)uiSize;
  info.m_uiAlignment = (ezUInt16)uiAlign;

  if (data.m_Flags.IsSet(ezMemoryTrackingFlags::EnableStackTrace))
  {
    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

    info.SetStackTrace(EZ_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces));
    ezMemoryUtils::Copy(info.GetStackTrace().GetPtr(), pBuffer, uiNumTraces);
  }

  EZ_VERIFY(!data.m_Allocations.Insert(ptr, info), "Allocation already known");
}

//static 
void ezMemoryTracker::RemoveAllocation(ezAllocatorId allocatorId, const void* ptr)
{
  EZ_LOCK(*s_pTrackerData);

  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  
  AllocationInfo info;
  if (data.m_Allocations.Remove(ptr, &info))
  {
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= (ezUInt64)info.m_uiSize;

    EZ_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid Allocation '%p'. Memory corruption?", ptr);
  }
}

//static
void ezMemoryTracker::RemoveAllAllocations(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= (ezUInt64)info.m_uiSize;

    EZ_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

//static
const char* ezMemoryTracker::GetAllocatorName(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName.GetData();
}

//static 
const ezAllocatorBase::Stats& ezMemoryTracker::GetAllocatorStats(ezAllocatorId allocatorId)
{
  EZ_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

//static 
const ezMemoryTracker::AllocationInfo& ezMemoryTracker::GetAllocationInfo(ezAllocatorId allocatorId, const void* ptr)
{
  EZ_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(ptr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  EZ_REPORT_FAILURE("Could not find info for allocation %p", ptr);
  return invalidInfo;
}


struct LeakInfo
{
  EZ_DECLARE_POD_TYPE();

  ezAllocatorId m_AllocatorId;
  size_t m_uiSize;
  const void* m_pParentLeak;

  EZ_FORCE_INLINE bool IsRootLeak() const
  {
    return m_pParentLeak == nullptr && m_AllocatorId != s_pTrackerData->m_StaticAllocatorId;
  }
};

//static
void ezMemoryTracker::DumpMemoryLeaks()
{
  if (s_pTrackerData == NULL) // if both tracking and tracing is disabled there is no tracker data
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
    const void* endPtr = ezMemoryUtils::AddByteOffsetConst(ptr, leak.m_uiSize);

    while (curPtr < endPtr)
    {
      const void* testPtr = *reinterpret_cast<const void* const*>(curPtr);

      LeakInfo* dependentLeak = nullptr;
      if (leakTable.TryGetValue(testPtr, dependentLeak))
      {
        dependentLeak->m_pParentLeak = ptr;
      }

      curPtr = ezMemoryUtils::AddByteOffsetConst(curPtr, sizeof(void*));
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
        PrintHelper(
          "\n\n--------------------------------------------------------------------\n"
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
    char szBuffer[512];
    ezStringUtils::snprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer),
      "\n--------------------------------------------------------------------\n"
      "Found %llu root memory leak(s)."
      "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);
    
    PrintHelper(szBuffer);

    EZ_REPORT_FAILURE("Found %llu root memory leak(s).", uiNumLeaks);
  }
}

//static 
ezMemoryTracker::Iterator ezMemoryTracker::GetIterator()
{
  auto pInnerIt = EZ_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_MemoryTracker);
