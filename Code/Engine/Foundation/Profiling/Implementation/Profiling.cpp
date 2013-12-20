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

  ON_BASE_SHUTDOWN
  {
    ezProfilingSystem::Shutdown();
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
    EZ_DECLARE_POD_TYPE();

    RefCountedProfilingInfo(const char* szName) : ProfilingInfo(szName)
    {
      m_uiRefCount = 1;
    }

    ezAtomicInteger32 m_uiRefCount;
  };

  typedef ezIdTable<ezProfilingId::InternalId, RefCountedProfilingInfo, 
    ezStaticAllocatorWrapper> ProfilingInfoTable;

  ProfilingInfoTable* g_pProfilingInfos;
  ezMutex g_ProfilingInfosMutex;

  ProfilingInfo& GetProfilingInfo(ezProfilingId::InternalId id)
  {
    return (*g_pProfilingInfos)[id];
  }
}

//static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
}

//static
void ezProfilingSystem::Shutdown()
{
}

//static
ezProfilingId ezProfilingSystem::CreateId(const char* szName)
{
  ezLock<ezMutex> lock(g_ProfilingInfosMutex);

  if (g_pProfilingInfos == NULL)
  {
    static ezUInt8 ProfilingInfosBuffer[sizeof(ProfilingInfoTable)];
    g_pProfilingInfos = new (ProfilingInfosBuffer) ProfilingInfoTable();
    g_pProfilingInfos->Reserve(EZ_PROFILING_ID_COUNT);
  }

  EZ_ASSERT(g_pProfilingInfos->GetCount() < EZ_PROFILING_ID_COUNT,
    "Max profiling id count (%d) reached. Increase EZ_PROFILING_ID_COUNT.", EZ_PROFILING_ID_COUNT);
  return ezProfilingId(g_pProfilingInfos->Insert(RefCountedProfilingInfo(szName)));
}

//static
void ezProfilingSystem::DeleteId(const ezProfilingId& id)
{
  ezLock<ezMutex> lock(g_ProfilingInfosMutex);

  g_pProfilingInfos->Remove(id.m_Id);
}

//static
void ezProfilingSystem::AddReference(const ezProfilingId& id)
{
  RefCountedProfilingInfo* pInfo;
  if (g_pProfilingInfos->TryGetValue(id.m_Id, pInfo))
  {
    pInfo->m_uiRefCount.Increment();
  }
}

//static
void ezProfilingSystem::ReleaseReference(const ezProfilingId& id)
{
  RefCountedProfilingInfo* pInfo;
  if (g_pProfilingInfos->TryGetValue(id.m_Id, pInfo))
  {
    if (pInfo->m_uiRefCount.Decrement() == 0)
    {
      ezLock<ezMutex> lock(g_ProfilingInfosMutex);
      g_pProfilingInfos->Remove(id.m_Id);
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

#endif


EZ_STATICLINK_REFPOINT(Foundation_Profiling_Implementation_Profiling);

