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
  typedef ezIdTable<ezProfilingId::InternalId, ProfilingInfo, ezStaticAllocatorWrapper> ProfilingInfoTable;

  ProfilingInfoTable* g_pProfilingInfos;
  ezMutex g_ProfilingInfosMutex;

  ProfilingInfo& GetProfilingInfo(ezProfilingId::InternalId id)
  {
    return (*g_pProfilingInfos)[id];
  }
}

void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
}

void ezProfilingSystem::Shutdown()
{
}

ezProfilingId ezProfilingSystem::CreateId(const char* szName)
{
  ezLock<ezMutex> lock(g_ProfilingInfosMutex);

  if (g_pProfilingInfos == NULL)
  {
    static ezUInt8 ProfilingInfosBuffer[sizeof(ProfilingInfoTable)];
    g_pProfilingInfos = new (ProfilingInfosBuffer) ProfilingInfoTable();
  }

  return ezProfilingId(g_pProfilingInfos->Insert(ProfilingInfo(szName)));
}

void ezProfilingSystem::DeleteId(const ezProfilingId& id)
{
  ezLock<ezMutex> lock(g_ProfilingInfosMutex);

  g_pProfilingInfos->Remove(id.m_Id);
}

#endif
