#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
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

ezCVarInt CVarMaxRenderViews(
  "r_ReflectionPoolMaxRenderViews", 6, ezCVarFlags::Default, "The maximum number of render views for reflection probes each frame");
ezCVarInt CVarMaxFilterViews(
  "r_ReflectionPoolMaxFilterViews", 1, ezCVarFlags::Default, "The maximum number of filter views for reflection probes each frame");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
// ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

namespace
{
  static ezUInt32 s_uiReflectionCubeMapSize = 128;

  struct ReflectionView
  {
    ezViewHandle m_hView;
    ezCamera m_Camera;
  };

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

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderPosX && value <= UpdateStep::RenderNegZ; }

    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo()
    {
      {
        ezGALTextureCreationDescription desc;
        desc.m_uiWidth = s_uiReflectionCubeMapSize;
        desc.m_uiHeight = s_uiReflectionCubeMapSize;
        desc.m_Format = ezGALResourceFormat::RGBAHalf;
        desc.m_Type = ezGALTextureType::TextureCube;
        desc.m_bCreateRenderTarget = true;
        desc.m_bAllowDynamicMipGeneration = true;

        m_hCubemap = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
      }

      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapViews); ++i)
      {
        ezGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture = m_hCubemap;
        desc.m_uiFirstSlice = i;
        desc.m_uiSliceCount = 1;

        m_hCubemapViews[i] = ezGALDevice::GetDefaultDevice()->CreateRenderTargetView(desc);
      }
    }

    ~ProbeUpdateInfo()
    {
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(m_hCubemap);
    }

    ezUInt64 m_uiLastActiveFrame = -1;
    ezUInt64 m_uiLastUpdatedFrame = -1;
    ezWorld* m_pWorld = nullptr;
    ezUInt16 m_uiPriority = 0;
    ezEnum<UpdateStep> m_LastUpdateStep;

    struct Step
    {
      EZ_DECLARE_POD_TYPE();

      ezUInt8 m_uiViewIndex;
      ezEnum<UpdateStep> m_UpdateStep;
    };

    ezHybridArray<Step, 8> m_UpdateSteps;

    ezGALTextureHandle m_hCubemap;
    ezGALRenderTargetViewHandle m_hCubemapViews[6];

    ezUInt32 GetPriority(ezUInt64 uiCurrentFrame) const
    {
      ezUInt32 uiFramesSinceLastUpdate = m_uiLastUpdatedFrame < uiCurrentFrame ? ezUInt32(uiCurrentFrame - m_uiLastUpdatedFrame) : 1000;
      return uiFramesSinceLastUpdate * m_uiPriority;
    }
  };

  struct SortedUpdateInfo
  {
    EZ_DECLARE_POD_TYPE();

    ProbeUpdateInfo* m_pUpdateInfo = nullptr;
    ezUInt32 m_uiIndex = 0;
    ezUInt32 m_uiPriority = 0;

    EZ_ALWAYS_INLINE bool operator<(const SortedUpdateInfo& other) const
    {
      if (m_uiPriority > other.m_uiPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }
  };

} // namespace

// must not be in anonymous namespace
template <>
struct ezHashHelper<ezReflectionProbeId>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezReflectionProbeId value) { return ezHashHelper<ezUInt32>::Hash(value.m_Data); }

  EZ_ALWAYS_INLINE static bool Equal(ezReflectionProbeId a, ezReflectionProbeId b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct ezReflectionPool::Data
{
  ~Data()
  {
    for (auto& renderView : m_RenderViews)
    {
      ezRenderWorld::DeleteView(renderView.m_hView);
    }

    for (auto& filterView : m_FilterViews)
    {
      ezRenderWorld::DeleteView(filterView.m_hView);
    }
  }

  void SortActiveProbes(const ezDynamicArray<ezReflectionProbeId>& activeProbes, ezUInt64 uiFrameCounter)
  {
    m_SortedUpdateInfo.Clear();

    for (ezUInt32 uiActiveProbeIndex = 0; uiActiveProbeIndex < activeProbes.GetCount(); ++uiActiveProbeIndex)
    {
      ProbeUpdateInfo* pUpdateInfo = nullptr;
      if (m_UpdateInfo.TryGetValue(activeProbes[uiActiveProbeIndex], pUpdateInfo))
      {
        auto& sorted = m_SortedUpdateInfo.ExpandAndGetRef();
        sorted.m_pUpdateInfo = pUpdateInfo;
        sorted.m_uiIndex = uiActiveProbeIndex;
        sorted.m_uiPriority = pUpdateInfo->GetPriority(uiFrameCounter);
      }
    }

    m_SortedUpdateInfo.Sort();
  }

  void GenerateUpdateSteps()
  {
    ezUInt32 uiRenderViewIndex = 0;
    ezUInt32 uiFilterViewIndex = 0;

    ezUInt32 uiSortedUpdateInfoIndex = 0;
    while (uiSortedUpdateInfoIndex < m_SortedUpdateInfo.GetCount())
    {
      const auto& sortedUpdateInfo = m_SortedUpdateInfo[uiSortedUpdateInfoIndex];
      auto pUpdateInfo = sortedUpdateInfo.m_pUpdateInfo;

      auto& updateSteps = pUpdateInfo->m_UpdateSteps;
      UpdateStep::Enum nextStep =
        UpdateStep::NextStep(updateSteps.IsEmpty() ? pUpdateInfo->m_LastUpdateStep : updateSteps.PeekBack().m_UpdateStep);

      bool bNextProbe = false;

      if (UpdateStep::IsRenderStep(nextStep))
      {
        if (uiRenderViewIndex < m_RenderViews.GetCount())
        {
          updateSteps.PushBack({(ezUInt8)uiRenderViewIndex, nextStep});
          ++uiRenderViewIndex;
        }
        else
        {
          bNextProbe = true;
        }
      }
      else if (nextStep == UpdateStep::Filter)
      {
        if (uiFilterViewIndex < m_FilterViews.GetCount())
        {
          updateSteps.PushBack({(ezUInt8)uiFilterViewIndex, nextStep});
          ++uiFilterViewIndex;
        }
        bNextProbe = true;
      }

      // break if no more views are available
      if (uiRenderViewIndex == m_RenderViews.GetCount() && uiFilterViewIndex == m_FilterViews.GetCount())
      {
        break;
      }

      // advance to next probe if it has the same priority as the current probe
      if (uiSortedUpdateInfoIndex + 1 < s_pData->m_SortedUpdateInfo.GetCount())
      {
        bNextProbe |= s_pData->m_SortedUpdateInfo[uiSortedUpdateInfoIndex + 1].m_uiPriority == sortedUpdateInfo.m_uiPriority;
      }

      if (bNextProbe)
      {
        ++uiSortedUpdateInfoIndex;
      }
    }
  }

  void CreateRenderViews()
  {
    ezUInt32 uiMaxRenderViews = ezMath::Max<ezUInt32>(CVarMaxRenderViews, 1);

    if (uiMaxRenderViews > m_RenderViews.GetCount())
    {
      ezStringBuilder sName;

      ezUInt32 uiCurrentCount = m_RenderViews.GetCount();
      for (ezUInt32 i = uiCurrentCount; i < uiMaxRenderViews; ++i)
      {
        auto& renderView = m_RenderViews.ExpandAndGetRef();

        sName.Format("Reflection Probe Render {}", i);

        ezView* pView = nullptr;
        renderView.m_hView = ezRenderWorld::CreateView(sName, pView);

        pView->SetCameraUsageHint(ezCameraUsageHint::Reflection);
        pView->SetViewport(ezRectFloat(0.0f, 0.0f, s_uiReflectionCubeMapSize, s_uiReflectionCubeMapSize));

        // ReflectionRenderPipeline.ezRenderPipelineAsset
        pView->SetRenderPipelineResource(
          ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 734898e8-b1a2-0da2-c4ae-701912983c2f }"));

        renderView.m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f); // TODO: expose
        pView->SetCamera(&renderView.m_Camera);
      }
    }
    else if (uiMaxRenderViews < m_RenderViews.GetCount())
    {
      m_RenderViews.SetCount(uiMaxRenderViews);
    }
  }

  void AddViewToRender(
    const ProbeUpdateInfo::Step& step, const ezReflectionProbeData& data, ProbeUpdateInfo& updateInfo, const ezVec3& vPosition)
  {
    ezVec3 vForward[6] = {
      ezVec3(1.0f, 0.0f, 0.0f),
      ezVec3(-1.0f, 0.0f, 0.0f),
      ezVec3(0.0f, 1.0f, 0.0f),
      ezVec3(0.0f, -1.0f, 0.0f),
      ezVec3(0.0f, 0.0f, 1.0f),
      ezVec3(0.0f, 0.0f, -1.0f),
    };

    ezVec3 vUp = ezVec3(0.0f, 0.0f, 1.0f);

    // Setup view and camera
    {
      ReflectionView* pReflectionView = nullptr;
      ezUInt32 uiFaceIndex = 0;

      if (step.m_UpdateStep == UpdateStep::Filter)
      {
        pReflectionView = &m_FilterViews[step.m_uiViewIndex];
      }
      else
      {
        pReflectionView = &m_RenderViews[step.m_uiViewIndex];
        uiFaceIndex = step.m_UpdateStep;
      }

      ezView* pView = nullptr;
      ezRenderWorld::TryGetView(pReflectionView->m_hView, pView);

      pView->m_IncludeTags = data.m_IncludeTags;
      pView->m_ExcludeTags = data.m_ExcludeTags;
      pView->SetWorld(updateInfo.m_pWorld);

      if (step.m_UpdateStep == UpdateStep::Filter)
      {
      }
      else
      {
        ezGALRenderTagetSetup renderTargetSetup;
        renderTargetSetup.SetRenderTarget(0, updateInfo.m_hCubemapViews[uiFaceIndex]);

        pView->SetRenderTargetSetup(renderTargetSetup);
      }

      pReflectionView->m_Camera.LookAt(vPosition, vPosition + vForward[uiFaceIndex], vUp);

      ezRenderWorld::AddViewToRender(pReflectionView->m_hView);
    }
  }

  ezDynamicArray<ReflectionView> m_RenderViews;
  ezDynamicArray<ReflectionView> m_FilterViews;

  ezIdTable<ezReflectionProbeId, ProbeUpdateInfo> m_UpdateInfo;

  ezMutex m_ActiveProbesMutex;
  ezDynamicArray<ezReflectionProbeId> m_ActiveDynamicProbes;

  ezDynamicArray<SortedUpdateInfo> m_SortedUpdateInfo;
};

ezReflectionPool::Data* ezReflectionPool::s_pData;

// static
void ezReflectionPool::RegisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld, ezUInt16 uiPriority /*= 1*/)
{
  ProbeUpdateInfo updateInfo;
  updateInfo.m_pWorld = pWorld;
  updateInfo.m_uiPriority = ezMath::Max<ezUInt16>(uiPriority, 1);

  data.m_Id = s_pData->m_UpdateInfo.Insert(std::move(updateInfo));

  s_pData->m_ActiveDynamicProbes.PushBack(data.m_Id);
}

void ezReflectionPool::DeregisterReflectionProbe(ezReflectionProbeData& data)
{
  s_pData->m_UpdateInfo.Remove(data.m_Id);
  data.m_Id.Invalidate();
}

// static
void ezReflectionPool::AddReflectionProbe(const ezReflectionProbeData& data, const ezVec3& vPosition, ezUInt16 uiPriority /*= 1*/)
{
  uiPriority = ezMath::Max<ezUInt16>(uiPriority, 1);

  ProbeUpdateInfo* pUpdateInfo = nullptr;
  if (!s_pData->m_UpdateInfo.TryGetValue(data.m_Id, pUpdateInfo))
    return;

  ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
  bool bExecuteUpdateSteps = false;

  {
    EZ_LOCK(s_pData->m_ActiveProbesMutex);

    pUpdateInfo->m_uiPriority = ezMath::Max(pUpdateInfo->m_uiPriority, uiPriority);

    if (pUpdateInfo->m_uiLastActiveFrame != uiCurrentFrame)
    {
      pUpdateInfo->m_uiLastActiveFrame = uiCurrentFrame;
      bExecuteUpdateSteps = !pUpdateInfo->m_UpdateSteps.IsEmpty();

      // Add as active for next frame
      s_pData->m_ActiveDynamicProbes.PushBack(data.m_Id);
    }
  }

  if (bExecuteUpdateSteps)
  {
    for (auto& step : pUpdateInfo->m_UpdateSteps)
    {
      s_pData->AddViewToRender(step, data, *pUpdateInfo, vPosition);
    }

    pUpdateInfo->m_LastUpdateStep = pUpdateInfo->m_UpdateSteps.PeekBack().m_UpdateStep;
    pUpdateInfo->m_UpdateSteps.Clear();

    pUpdateInfo->m_uiLastUpdatedFrame = uiCurrentFrame;
  }
}

// static
void ezReflectionPool::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(ezReflectionPool::Data);

  ezRenderWorld::s_BeginExtractionEvent.AddEventHandler(OnBeginExtraction);
  ezRenderWorld::s_EndExtractionEvent.AddEventHandler(OnEndExtraction);
}

// static
void ezReflectionPool::OnEngineShutdown()
{
  ezRenderWorld::s_BeginExtractionEvent.RemoveEventHandler(OnBeginExtraction);
  ezRenderWorld::s_EndExtractionEvent.RemoveEventHandler(OnEndExtraction);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezReflectionPool::OnBeginExtraction(ezUInt64 uiFrameCounter)
{
  EZ_PROFILE_SCOPE("Reflection Pool Update");

  s_pData->CreateRenderViews();

  // generate possible update steps for active probes
  {
    s_pData->SortActiveProbes(s_pData->m_ActiveDynamicProbes, uiFrameCounter);

    s_pData->GenerateUpdateSteps();

    s_pData->m_ActiveDynamicProbes.Clear();
  }
}

// static
void ezReflectionPool::OnEndExtraction(ezUInt64 uiFrameCounter) {}
