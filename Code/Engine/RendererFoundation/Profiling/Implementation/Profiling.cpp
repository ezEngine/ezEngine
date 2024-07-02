#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

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

    while (!s_TimingScopes.IsEmpty())
    {
      auto& timingScope = s_TimingScopes.PeekFront();

      ezTime beginTime;
      ezTime endTime;
      ezEnum<ezGALAsyncResult> resBegin = e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime);
      ezEnum<ezGALAsyncResult> resEnd = e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime);

      if (resBegin == ezGALAsyncResult::Expired || resEnd == ezGALAsyncResult::Expired)
      {
        s_TimingScopes.PopFront();
      }

      if (resBegin == ezGALAsyncResult::Ready && resEnd == ezGALAsyncResult::Ready)
      {
        if (!beginTime.IsZero() && !endTime.IsZero())
        {
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
          static bool warnOnRingBufferOverun = true;
          if (warnOnRingBufferOverun && endTime < beginTime)
          {
            warnOnRingBufferOverun = false;
            ezLog::Warning("Profiling end is before start, the DX11 timestamp ring buffer was probably overrun.");
          }
#  endif
          ezProfilingSystem::AddGPUScope(timingScope.m_szName, beginTime, endTime);
        }

        s_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope() { return s_TimingScopes.ExpandAndGetRef(); }

private:
  static void OnEngineStartup() { ezGALDevice::GetDefaultDevice()->s_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps); }

  static void OnEngineShutdown()
  {
    s_TimingScopes.Clear();
    ezGALDevice::GetDefaultDevice()->s_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static ezDeque<GPUTimingScope, ezStaticsAllocatorWrapper> s_TimingScopes;

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

ezDeque<GPUTimingScope, ezStaticsAllocatorWrapper> GPUProfilingSystem::s_TimingScopes;

//////////////////////////////////////////////////////////////////////////

GPUTimingScope* ezProfilingScopeAndMarker::Start(ezGALCommandEncoder* pCommandEncoder, const char* szName)
{
  pCommandEncoder->PushMarker(szName);

  auto& timingScope = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = pCommandEncoder->InsertTimestamp();
  ezStringUtils::Copy(timingScope.m_szName, EZ_ARRAY_SIZE(timingScope.m_szName), szName);

  return &timingScope;
}

void ezProfilingScopeAndMarker::Stop(ezGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope)
{
  pCommandEncoder->PopMarker();
  ref_pTimingScope->m_EndTimestamp = pCommandEncoder->InsertTimestamp();
  ref_pTimingScope = nullptr;
}

ezProfilingScopeAndMarker::ezProfilingScopeAndMarker(ezGALCommandEncoder* pCommandEncoder, const char* szName)
  : ezProfilingScope(szName, nullptr, ezTime::MakeZero())
  , m_pCommandEncoder(pCommandEncoder)
{
  m_pTimingScope = Start(pCommandEncoder, szName);
}

ezProfilingScopeAndMarker::~ezProfilingScopeAndMarker()
{
  Stop(m_pCommandEncoder, m_pTimingScope);
}

#endif

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);
