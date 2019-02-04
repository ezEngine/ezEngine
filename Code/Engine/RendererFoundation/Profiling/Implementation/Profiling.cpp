#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

struct GPUTimingScope
{
  EZ_DECLARE_POD_TYPE();

  ezGALTimestampHandle m_BeginTimestamp;
  ezGALTimestampHandle m_EndTimestamp;
  char m_szName[48];
};

class GPUProfilingSystem
{
public:
  static void ProcessTimestamps(const ezGALDeviceEvent& e)
  {
    if (e.m_Type != ezGALDeviceEvent::AfterEndFrame)
      return;

    while (!m_TimingScopes.IsEmpty())
    {
      auto& timingScope = m_TimingScopes.PeekFront();

      ezTime endTime;
      if (e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime).Succeeded())
      {
        ezTime beginTime;
        EZ_VERIFY(e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime).Succeeded(),
          "Begin timestamp should be finished before end timestamp");

        if (!beginTime.IsZero() && !endTime.IsZero())
        {
          auto& gpuData = ezProfilingSystem::AllocateGPUData();
          gpuData.m_BeginTime = beginTime;
          gpuData.m_EndTime = endTime;
          ezMemoryUtils::Copy(gpuData.m_szName, timingScope.m_szName, EZ_ARRAY_SIZE(gpuData.m_szName));
        }

        m_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope()
  {
    return m_TimingScopes.ExpandAndGetRef();
  }

private:
  static void OnEngineStartup()
  {
    ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static void OnEngineShutdown()
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static ezDeque<GPUTimingScope, ezStaticAllocatorWrapper> m_TimingScopes;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, GPUProfilingSystem);
};

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, GPUProfilingSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    GPUProfilingSystem::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    GPUProfilingSystem::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezDeque<GPUTimingScope, ezStaticAllocatorWrapper> GPUProfilingSystem::m_TimingScopes;

//////////////////////////////////////////////////////////////////////////

ezProfilingScopeAndMarker::ezProfilingScopeAndMarker(ezGALContext* pGALContext, const char* szName)
    : ezProfilingScope(szName, nullptr)
    , m_pGALContext(pGALContext)
{
  m_pGALContext->PushMarker(m_szName);

  auto& timingScope = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = m_pGALContext->InsertTimestamp();
  ezStringUtils::CopyN(timingScope.m_szName, EZ_ARRAY_SIZE(timingScope.m_szName), m_szName, m_uiNameLength);

  m_pTimingScope = &timingScope;
}

ezProfilingScopeAndMarker::~ezProfilingScopeAndMarker()
{
  m_pGALContext->PopMarker();
  m_pTimingScope->m_EndTimestamp = m_pGALContext->InsertTimestamp();
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);

