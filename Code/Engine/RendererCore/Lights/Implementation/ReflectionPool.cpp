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
  static ezUInt32 s_uiNumReflectionProbeCubeMaps = 32;

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
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }

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
        desc.m_uiMipLevelCount = ezMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
        desc.m_Format = ezGALResourceFormat::RGBAHalf;
        desc.m_Type = ezGALTextureType::TextureCube;
        desc.m_bCreateRenderTarget = true;
        desc.m_bAllowDynamicMipGeneration = true;

        m_hCubemap = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
      }

      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
      {
        m_hCubemapProxies[i] = ezGALDevice::GetDefaultDevice()->CreateProxyTexture(m_hCubemap, i);
      }
    }

    ProbeUpdateInfo(ProbeUpdateInfo&& other) { *this = std::move(other); }

    ~ProbeUpdateInfo()
    {
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
      {
        if (!m_hCubemapProxies[i].IsInvalidated())
        {
          ezGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hCubemapProxies[i]);
        }
      }

      if (!m_hCubemap.IsInvalidated())
      {
        ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(m_hCubemap);
      }
    }

    void operator=(ProbeUpdateInfo&& other)
    {
      m_uiLastActiveFrame = other.m_uiLastActiveFrame;
      m_uiLastUpdatedFrame = other.m_uiLastUpdatedFrame;
      m_pWorld = other.m_pWorld;
      m_LastUpdateStep = other.m_LastUpdateStep;

      m_UpdateSteps = std::move(other.m_UpdateSteps);

      m_hCubemap = other.m_hCubemap;
      other.m_hCubemap.Invalidate();

      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
      {
        m_hCubemapProxies[i] = other.m_hCubemapProxies[i];
        other.m_hCubemapProxies[i].Invalidate();
      }
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
    ezGALTextureHandle m_hCubemapProxies[6];

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

  static void CreateViews(
    ezDynamicArray<ReflectionView>& views, ezUInt32 uiMaxRenderViews, const char* szNameSuffix, const char* szRenderPipelineResource)
  {
    uiMaxRenderViews = ezMath::Max<ezUInt32>(uiMaxRenderViews, 1);

    if (uiMaxRenderViews > views.GetCount())
    {
      ezStringBuilder sName;

      ezUInt32 uiCurrentCount = views.GetCount();
      for (ezUInt32 i = uiCurrentCount; i < uiMaxRenderViews; ++i)
      {
        auto& renderView = views.ExpandAndGetRef();

        sName.Format("Reflection Probe {} {}", szNameSuffix, i);

        ezView* pView = nullptr;
        renderView.m_hView = ezRenderWorld::CreateView(sName, pView);

        pView->SetCameraUsageHint(ezCameraUsageHint::Reflection);
        pView->SetViewport(ezRectFloat(0.0f, 0.0f, s_uiReflectionCubeMapSize, s_uiReflectionCubeMapSize));

        pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>(szRenderPipelineResource));

        renderView.m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f); // TODO: expose
        pView->SetCamera(&renderView.m_Camera);
      }
    }
    else if (uiMaxRenderViews < views.GetCount())
    {
      views.SetCount(uiMaxRenderViews);
    }
  }

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

    if (!m_hReflectionSpecularTexture.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hReflectionSpecularTexture);
      m_hReflectionSpecularTexture.Invalidate();
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

  void CreateViewsAndResources()
  {
    // ReflectionRenderPipeline.ezRenderPipelineAsset
    CreateViews(m_RenderViews, CVarMaxRenderViews, "Render", "{ 734898e8-b1a2-0da2-c4ae-701912983c2f }");

    // ReflectionFilterPipeline.ezRenderPipelineAsset
    CreateViews(m_FilterViews, CVarMaxFilterViews, "Filter", "{ 3437db17-ddf1-4b67-b80f-9999d6b0c352 }");

    if (m_hReflectionSpecularTexture.IsInvalidated())
    {
      ezGALTextureCreationDescription desc;
      desc.m_uiWidth = s_uiReflectionCubeMapSize;
      desc.m_uiHeight = s_uiReflectionCubeMapSize;
      desc.m_uiMipLevelCount = ezMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
      desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
      desc.m_Format = ezGALResourceFormat::RGBAHalf;
      desc.m_Type = ezGALTextureType::TextureCube;
      desc.m_bCreateRenderTarget = true;

      m_hReflectionSpecularTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  void AddViewToRender(
    const ProbeUpdateInfo::Step& step, const ezReflectionProbeData& data, ProbeUpdateInfo& updateInfo, const ezVec3& vPosition)
  {
    ezVec3 vForward[6] = {
      ezVec3(1.0f, 0.0f, 0.0f),
      ezVec3(-1.0f, 0.0f, 0.0f),
      ezVec3(0.0f, 0.0f, 1.0f),
      ezVec3(0.0f, 0.0f, -1.0f),
      ezVec3(0.0f, -1.0f, 0.0f),
      ezVec3(0.0f, 1.0f, 0.0f),
    };

    ezVec3 vUp[6] = {
      ezVec3(0.0f, 0.0f, 1.0f),
      ezVec3(0.0f, 0.0f, 1.0f),
      ezVec3(0.0f, 1.0f, 0.0f),
      ezVec3(0.0f, -1.0f, 0.0f),
      ezVec3(0.0f, 0.0f, 1.0f),
      ezVec3(0.0f, 0.0f, 1.0f),
    };

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

      ezGALRenderTargetSetup renderTargetSetup;
      if (step.m_UpdateStep == UpdateStep::Filter)
      {
        renderTargetSetup.SetRenderTarget(0, ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hReflectionSpecularTexture));

        pView->SetRenderPassProperty("ReflectionFilterPass", "InputCubemap", updateInfo.m_hCubemap.GetInternalID().m_Data);
      }
      else
      {
        renderTargetSetup.SetRenderTarget(
          0, ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(updateInfo.m_hCubemapProxies[uiFaceIndex]));
      }
      pView->SetRenderTargetSetup(renderTargetSetup);

      pReflectionView->m_Camera.LookAt(vPosition, vPosition + vForward[uiFaceIndex], vUp[uiFaceIndex]);

      ezRenderWorld::AddViewToRender(pReflectionView->m_hView);
    }
  }

  ezDynamicArray<ReflectionView> m_RenderViews;
  ezDynamicArray<ReflectionView> m_FilterViews;

  ezIdTable<ezReflectionProbeId, ProbeUpdateInfo> m_UpdateInfo;

  ezMutex m_ActiveProbesMutex;
  ezDynamicArray<ezReflectionProbeId> m_ActiveDynamicProbes;

  ezDynamicArray<SortedUpdateInfo> m_SortedUpdateInfo;

  ezGALTextureHandle m_hReflectionSpecularTexture;
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
    for (ezUInt32 i = pUpdateInfo->m_UpdateSteps.GetCount(); i-- > 0;)
    {
      s_pData->AddViewToRender(pUpdateInfo->m_UpdateSteps[i], data, *pUpdateInfo, vPosition);
    }

    pUpdateInfo->m_LastUpdateStep = pUpdateInfo->m_UpdateSteps.PeekBack().m_UpdateStep;
    pUpdateInfo->m_UpdateSteps.Clear();

    pUpdateInfo->m_uiLastUpdatedFrame = uiCurrentFrame;
  }
}

// static
ezUInt32 ezReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

ezGALTextureHandle ezReflectionPool::GetReflectionSpecularTexture()
{
  return s_pData->m_hReflectionSpecularTexture;
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

  if (s_pData->m_ActiveDynamicProbes.IsEmpty())
  {
    return;
  }

  s_pData->CreateViewsAndResources();

  // generate possible update steps for active probes
  {
    s_pData->SortActiveProbes(s_pData->m_ActiveDynamicProbes, uiFrameCounter);

    s_pData->GenerateUpdateSteps();

    s_pData->m_ActiveDynamicProbes.Clear();
  }
}

// static
void ezReflectionPool::OnEndExtraction(ezUInt64 uiFrameCounter) {}
