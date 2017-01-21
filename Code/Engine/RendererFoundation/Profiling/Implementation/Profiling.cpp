#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Profiling/GPUStopwatch.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/Stats.h>

namespace
{
  ezTypedCVar<bool, ezCVarType::Bool> s_enableGPUProfiling("Enable GPU profiling", false, ezCVarFlags::None, "If true, every ezProfilingScopeAndMarker scope uses a ezGPUStopwatch to measure the passed GPU time and writes the result in ms to ezStats");
  ezHybridArray<ezString, 4> s_previousProfilingScopes;
}

ezProfilingScopeAndMarker::ezProfilingScopeAndMarker(ezGALContext* pGALContext, const char* szName, const char* szFunctionName)
  : ezProfilingScope(szName, szFunctionName)
  , m_pGALContext(pGALContext)
{
  m_pGALContext->PushMarker(m_szName);
  s_previousProfilingScopes.PushBack(m_szName);

  if (s_enableGPUProfiling.GetValue())
  {
    ezGPUStopwatch& stopwatch = m_pGALContext->GetDevice()->GetOrCreateGPUStopwatch(szName);
    stopwatch.Begin(*m_pGALContext);
  }
}

ezProfilingScopeAndMarker::~ezProfilingScopeAndMarker()
{
  m_pGALContext->PopMarker();
  s_previousProfilingScopes.PopBack();

  if (s_enableGPUProfiling.GetValue())
  {
    ezGPUStopwatch& stopwatch = m_pGALContext->GetDevice()->GetOrCreateGPUStopwatch(m_szName);
    if (stopwatch.IsRunning())
    {
      ezTime time;
      if (stopwatch.End(*m_pGALContext, &time).Succeeded())
      {
        ezStringBuilder statString("GPU/");
        for (const ezString& prevScope : s_previousProfilingScopes)
          statString.Append(prevScope, "/");
        statString.Append(m_szName);

        ezStats::SetStat(statString, ezConversionUtils::ToString(time.GetMilliseconds()));
      }
    }
  }
}
