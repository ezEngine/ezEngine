#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  ezReflectionPool::OnEngineStartup();
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  ezReflectionPool::OnEngineShutdown();
}

EZ_END_SUBSYSTEM_DECLARATION;
  // clang-format on

ezCVarInt CVArMaxRenderViews("r_ReflectionPoolMaxRenderViews", 6, ezCVarFlags::Default, "The maximum number of render views for reflection probes each frame");
ezCVarInt CVArMaxFilterViews("r_ReflectionPoolMaxFilterViews", 1, ezCVarFlags::Default, "The maximum number of filter views for reflection probes each frame");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
// ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

namespace
{
  struct UpdateStep
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      RenderPosX,
      RenderNegX,
      RenderPosY,
      RenderNegY,
      RenderPosZ,
      RenderNegZ,
      Filter,

      Default = RenderPosX
    };
  };

  struct ProbeUpdateInfo
  {
    ezUInt64 m_uiLastActiveFrame = -1;
    ezUInt64 m_uiLastUpdatedFrame = -1;
    const ezWorld* m_pWorld = nullptr;
    ezVec3 m_vPosition;
    ezUInt16 m_uiPriority = 0;
    ezEnum<UpdateStep> m_NextUpdateStep;

    ezUInt32 GetPriority(ezUInt64 uiCurrentFrame) const
    {
      ezUInt32 uiFramesSinceLastUpdate = m_uiLastUpdatedFrame < uiCurrentFrame ? ezUInt32(uiCurrentFrame - m_uiLastUpdatedFrame) : 1000;
      return uiFramesSinceLastUpdate * m_uiPriority;
    }

    bool NextUpdateStepIsRender() { return m_NextUpdateStep >= UpdateStep::RenderPosX && m_NextUpdateStep <= UpdateStep::RenderNegZ; }

    bool NextUpdateStepIsFilter() { return m_NextUpdateStep == UpdateStep::Filter; }
  };

  struct ActiveProbe
  {
    EZ_DECLARE_POD_TYPE();

    const ezReflectionProbeData* m_pData;
    ProbeUpdateInfo* m_pUpdateInfo;
  };

  struct SortedUpdateInfo
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiIndex;
    ezUInt32 m_uiPriority;

    EZ_ALWAYS_INLINE bool operator<(const SortedUpdateInfo& other) const
    {
      if (m_uiPriority > other.m_uiPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }
  };

  static void SortActiveProbes(
    ezDynamicArray<SortedUpdateInfo>& sortedUpdateInfo, const ezDynamicArray<ActiveProbe>& activeProbes, ezUInt64 uiFrameCounter)
  {
    sortedUpdateInfo.Clear();

    for (ezUInt32 uiActiveProbeIndex = 0; uiActiveProbeIndex < activeProbes.GetCount(); ++uiActiveProbeIndex)
    {
      auto& activeProbe = activeProbes[uiActiveProbeIndex];

      auto& sorted = sortedUpdateInfo.ExpandAndGetRef();
      sorted.m_uiIndex = uiActiveProbeIndex;
      sorted.m_uiPriority = activeProbe.m_pUpdateInfo->GetPriority(uiFrameCounter);
    }

    sortedUpdateInfo.Sort();
  }

} // namespace

struct ezReflectionPool::Data
{
  void Clear()
  {
    m_ActiveProbesToRender.Clear();
    m_ActiveProbesToFilter.Clear();
  }

  ezIdTable<ezReflectionProbeId, ProbeUpdateInfo> m_UpdateInfo;

  ezMutex m_ActiveProbesMutex;
  ezDynamicArray<ActiveProbe> m_ActiveProbesToRender;
  ezDynamicArray<ActiveProbe> m_ActiveProbesToFilter;

  ezDynamicArray<SortedUpdateInfo> m_SortedUpdateInfo;
};

ezReflectionPool::Data* ezReflectionPool::s_pData;

// static
void ezReflectionPool::RegisterReflectionProbe(ezReflectionProbeData& data)
{
  ProbeUpdateInfo updateInfo;

  data.m_Id = s_pData->m_UpdateInfo.Insert(std::move(updateInfo));
}

void ezReflectionPool::DeregisterReflectionProbe(ezReflectionProbeData& data)
{
  s_pData->m_UpdateInfo.Remove(data.m_Id);
  data.m_Id.Invalidate();
}

// static
void ezReflectionPool::AddReflectionProbe(
  const ezReflectionProbeData& data, const ezWorld* pWorld, const ezVec3& vPosition, ezUInt16 uiPriority /*= 1*/)
{
  ProbeUpdateInfo* pUpdateInfo = nullptr;
  if (s_pData->m_UpdateInfo.TryGetValue(data.m_Id, pUpdateInfo))
  {
    ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();

    EZ_LOCK(s_pData->m_ActiveProbesMutex);

    if (pUpdateInfo->m_uiLastActiveFrame != uiCurrentFrame)
    {
      pUpdateInfo->m_uiLastActiveFrame = uiCurrentFrame;
      pUpdateInfo->m_pWorld = pWorld;
      pUpdateInfo->m_vPosition = vPosition;
      pUpdateInfo->m_uiPriority = ezMath::Max<ezUInt16>(uiPriority, 1);

      ActiveProbe activeProbe;
      activeProbe.m_pData = &data;
      activeProbe.m_pUpdateInfo = pUpdateInfo;

      if (pUpdateInfo->NextUpdateStepIsRender())
      {
        s_pData->m_ActiveProbesToRender.PushBack(std::move(activeProbe));
      }
      else if (pUpdateInfo->NextUpdateStepIsFilter())
      {
        s_pData->m_ActiveProbesToFilter.PushBack(std::move(activeProbe));
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }
}

// static
void ezReflectionPool::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(ezReflectionPool::Data);

  ezRenderWorld::s_EndExtractionEvent.AddEventHandler(OnEndExtraction);
}

// static
void ezReflectionPool::OnEngineShutdown()
{
  ezRenderWorld::s_EndExtractionEvent.RemoveEventHandler(OnEndExtraction);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezReflectionPool::OnEndExtraction(ezUInt64 uiFrameCounter)
{
  EZ_PROFILE_SCOPE("Reflection Pool Update");

  RenderActiveProbes(uiFrameCounter);
  FilterActiveProbes(uiFrameCounter);

  s_pData->Clear();
}

void ezReflectionPool::RenderActiveProbes(ezUInt64 uiFrameCounter)
{
  EZ_PROFILE_SCOPE("Render Active Probes");

  SortActiveProbes(s_pData->m_SortedUpdateInfo, s_pData->m_ActiveProbesToRender, uiFrameCounter);
}

void ezReflectionPool::FilterActiveProbes(ezUInt64 uiFrameCounter)
{
  EZ_PROFILE_SCOPE("Filter Active Probes");

  SortActiveProbes(s_pData->m_SortedUpdateInfo, s_pData->m_ActiveProbesToFilter, uiFrameCounter);
}
