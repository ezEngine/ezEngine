#include <Foundation/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezProfilingSystem::Initialize();
  }

EZ_END_SUBSYSTEM_DECLARATION

namespace
{
  struct ProfilingInfo;
  ProfilingInfo& GetProfilingInfo(ezProfilingId::InternalId id);
}

// Include inline file
#if EZ_ENABLED(EZ_USE_PROFILING_GPA) && EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Profiling/Implementation/Profiling_GPA_inl.h>
#else
  #include <Foundation/Profiling/Implementation/Profiling_EZ_inl.h>
#endif

namespace
{
  struct RefCountedProfilingInfo : public ProfilingInfo
  {
    RefCountedProfilingInfo(const char* szName) : ProfilingInfo(szName)
    {
      m_uiRefCount = 1;
    }

    ezAtomicInteger32 m_uiRefCount;
  };

  struct ProfilingData
  {
    EZ_FORCE_INLINE void Acquire() { m_Mutex.Acquire(); }
    EZ_FORCE_INLINE void Release() { m_Mutex.Release(); }

    ezMutex m_Mutex;

    ezIdTable<ezProfilingId::InternalId, RefCountedProfilingInfo> m_InfoTable;
  };

  static ProfilingData* s_pProfilingData;
  
  static bool InitializeData()
  {
    if (s_pProfilingData == nullptr)
    {
      static ezUInt8 ProfilingDataBuffer[sizeof(ProfilingData)];
      s_pProfilingData = new (ProfilingDataBuffer) ProfilingData();
      s_pProfilingData->m_InfoTable.Reserve(EZ_PROFILING_ID_COUNT);
    }
    return true;
  }

  // using this static dummy variable we make sure that the profiling info table uses the static allocator
  static bool s_bDummy = InitializeData();

  ProfilingInfo& GetProfilingInfo(ezProfilingId::InternalId id)
  {
    return s_pProfilingData->m_InfoTable[id];
  }
}

//static
void ezProfilingSystem::Initialize()
{
  EZ_ASSERT_DEV(s_pProfilingData != nullptr, "Profiling Data should already be initialized");
  EZ_ASSERT_DEV(s_pProfilingData->m_InfoTable.GetAllocator() != ezFoundation::GetDefaultAllocator(), "Profiling Data must use the static allocator");

  SetThreadName("Main Thread");
}

//static
ezProfilingId ezProfilingSystem::CreateId(const char* szName)
{
  InitializeData();

  EZ_LOCK(*s_pProfilingData);

  EZ_ASSERT_DEV(s_pProfilingData->m_InfoTable.GetCount() < EZ_PROFILING_ID_COUNT,
    "Max profiling id count (%d) reached. Increase EZ_PROFILING_ID_COUNT.", EZ_PROFILING_ID_COUNT);
  return ezProfilingId(s_pProfilingData->m_InfoTable.Insert(RefCountedProfilingInfo(szName)));
}

//static
void ezProfilingSystem::DeleteId(const ezProfilingId& id)
{
  EZ_LOCK(*s_pProfilingData);

  s_pProfilingData->m_InfoTable.Remove(id.m_Id);
}

//static
void ezProfilingSystem::AddReference(const ezProfilingId& id)
{
  RefCountedProfilingInfo* pInfo;
  if (s_pProfilingData->m_InfoTable.TryGetValue(id.m_Id, pInfo))
  {
    pInfo->m_uiRefCount.Increment();
  }
}

//static
void ezProfilingSystem::ReleaseReference(const ezProfilingId& id)
{
  // profiling system already de-initialized, nothing to do anymore. Can happen during static de-initialization.
  if (s_pProfilingData == nullptr)
    return;

  RefCountedProfilingInfo* pInfo;
  if (s_pProfilingData->m_InfoTable.TryGetValue(id.m_Id, pInfo))
  {
    if (pInfo->m_uiRefCount.Decrement() == 0)
    {
      EZ_LOCK(*s_pProfilingData);
      s_pProfilingData->m_InfoTable.Remove(id.m_Id);
    }
  }
}

#else

//static
ezProfilingId ezProfilingSystem::CreateId(const char* szName)
{
  return ezProfilingId();
}

//static
void ezProfilingSystem::DeleteId(const ezProfilingId& id)
{
}

//static
void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
}

void ezProfilingSystem::Initialize()
{
}

void ezProfilingSystem::AddReference(const ezProfilingId& id)
{
}

void ezProfilingSystem::ReleaseReference(const ezProfilingId& id)
{
}

void ezProfilingSystem::Capture(ezStreamWriterBase& outputStream)
{
}

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);

