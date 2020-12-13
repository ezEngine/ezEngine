#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
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
  "r_ReflectionPoolMaxRenderViews", 1, ezCVarFlags::Default, "The maximum number of render views for reflection probes each frame");
ezCVarInt CVarMaxFilterViews(
  "r_ReflectionPoolMaxFilterViews", 1, ezCVarFlags::Default, "The maximum number of filter views for reflection probes each frame");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
// ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

static ezUInt32 s_uiReflectionCubeMapSize = 128;
static ezUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static float s_fDebugSphereRadius = 0.3f;

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
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

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
      pDevice->GetTexture(m_hCubemap)->SetDebugName("Reflection Cubemap");
    }

    ezStringBuilder sName;
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
    {
      m_hCubemapProxies[i] = ezGALDevice::GetDefaultDevice()->CreateProxyTexture(m_hCubemap, i);

      sName.Format("Reflection Cubemap Proxy {}", i);
      pDevice->GetTexture(m_hCubemapProxies[i])->SetDebugName(sName);
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
  float m_fPriority = 0.0f;
  ezUInt32 m_uiIndexInUpdateQueue = 0;
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

  float GetPriority(ezUInt64 uiCurrentFrame) const
  {
    ezUInt32 uiFramesSinceLastUpdate = m_uiLastUpdatedFrame < uiCurrentFrame ? ezUInt32(uiCurrentFrame - m_uiLastUpdatedFrame) : 10000;
    return uiFramesSinceLastUpdate * m_fPriority;
  }
};

struct SortedUpdateInfo
{
  EZ_DECLARE_POD_TYPE();

  ProbeUpdateInfo* m_pUpdateInfo = nullptr;
  ezUInt32 m_uiIndex = 0;
  float m_fPriority = 0.0f;

  EZ_ALWAYS_INLINE bool operator<(const SortedUpdateInfo& other) const
  {
    if (m_fPriority > other.m_fPriority) // we want to sort descending (higher priority first)
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
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, static_cast<float>(s_uiReflectionCubeMapSize), static_cast<float>(s_uiReflectionCubeMapSize)));

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
  Data() { m_SkyIrradianceStorage.SetCount(64); }

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

    if (!m_hSkyIrradianceTexture.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
      m_hSkyIrradianceTexture.Invalidate();
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
        sorted.m_fPriority = pUpdateInfo->GetPriority(uiFrameCounter);
      }
    }

    m_SortedUpdateInfo.Sort();

    for (ezUInt32 i = 0; i < m_SortedUpdateInfo.GetCount(); ++i)
    {
      m_SortedUpdateInfo[i].m_pUpdateInfo->m_uiIndexInUpdateQueue = i;
    }
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
      UpdateStep::Enum nextStep = UpdateStep::NextStep(updateSteps.IsEmpty() ? pUpdateInfo->m_LastUpdateStep : updateSteps.PeekBack().m_UpdateStep);

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
        // don't do render and filter in one frame
        if (uiFilterViewIndex < m_FilterViews.GetCount() && updateSteps.IsEmpty())
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
        bNextProbe |= s_pData->m_SortedUpdateInfo[uiSortedUpdateInfoIndex + 1].m_fPriority == sortedUpdateInfo.m_fPriority;
      }

      if (bNextProbe)
      {
        ++uiSortedUpdateInfoIndex;
      }
    }
  }

  void CreateReflectionViewsAndResources()
  {
    // ReflectionRenderPipeline.ezRenderPipelineAsset
    CreateViews(m_RenderViews, CVarMaxRenderViews, "Render", "{ 734898e8-b1a2-0da2-c4ae-701912983c2f }");

    // ReflectionFilterPipeline.ezRenderPipelineAsset
    CreateViews(m_FilterViews, CVarMaxFilterViews, "Filter", "{ 3437db17-ddf1-4b67-b80f-9999d6b0c352 }");

    if (m_hReflectionSpecularTexture.IsInvalidated())
    {
      ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

      ezGALTextureCreationDescription desc;
      desc.m_uiWidth = s_uiReflectionCubeMapSize;
      desc.m_uiHeight = s_uiReflectionCubeMapSize;
      desc.m_uiMipLevelCount = ezMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
      desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
      desc.m_Format = ezGALResourceFormat::RGBAHalf;
      desc.m_Type = ezGALTextureType::TextureCube;
      desc.m_bCreateRenderTarget = true;

      m_hReflectionSpecularTexture = pDevice->CreateTexture(desc);
      pDevice->GetTexture(m_hReflectionSpecularTexture)->SetDebugName("Reflection Specular Texture");
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (!m_hDebugSphere.IsValid())
    {
      ezGeometry geom;
      geom.AddSphere(s_fDebugSphereRadius, 32, 16, ezColor::White);

      const char* szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
      if (!hMeshBuffer.IsValid())
      {
        ezMeshBufferResourceDescriptor desc;
        desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
        desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
      }

      const char* szMeshResourceName = "ReflectionProbeDebugSphere";
      m_hDebugSphere = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
      if (!m_hDebugSphere.IsValid())
      {
        ezMeshResourceDescriptor desc;
        desc.UseExistingMeshBuffer(hMeshBuffer);
        desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
        desc.ComputeBounds();

        m_hDebugSphere = ezResourceManager::CreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
      }
    }

    if (!m_hDebugMaterial.IsValid())
    {
      m_hDebugMaterial = ezResourceManager::LoadResource<ezMaterialResource>(
        "{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.ezMaterialAsset
    }
#endif
  }

  void CreateSkyIrradianceTexture()
  {
    if (m_hSkyIrradianceTexture.IsInvalidated())
    {
      ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

      ezGALTextureCreationDescription desc;
      desc.m_uiWidth = 6;
      desc.m_uiHeight = 64;
      desc.m_Format = ezGALResourceFormat::RGBAHalf;
      desc.m_Type = ezGALTextureType::Texture2D;
      desc.m_bCreateRenderTarget = true;
      desc.m_bAllowUAV = true;

      m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);
      pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
    }
  }

  void AddViewToRender(const ProbeUpdateInfo::Step& step, const ezReflectionProbeData& data, ProbeUpdateInfo& updateInfo, const ezVec3& vPosition)
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

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

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
        renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hReflectionSpecularTexture));
        renderTargetSetup.SetRenderTarget(2, pDevice->GetDefaultRenderTargetView(m_hSkyIrradianceTexture));

        pView->SetRenderPassProperty("ReflectionFilterPass", "Intensity", data.m_fIntensity);
        pView->SetRenderPassProperty("ReflectionFilterPass", "Saturation", data.m_fSaturation);
        pView->SetRenderPassProperty("ReflectionFilterPass", "OutputIndex", updateInfo.m_pWorld->GetIndex());
        pView->SetRenderPassProperty("ReflectionFilterPass", "InputCubemap", updateInfo.m_hCubemap.GetInternalID().m_Data);
      }
      else
      {
        renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(updateInfo.m_hCubemapProxies[uiFaceIndex]));
      }
      pView->SetRenderTargetSetup(renderTargetSetup);

      pReflectionView->m_Camera.LookAt(vPosition, vPosition + vForward[uiFaceIndex], vUp[uiFaceIndex]);

      ezRenderWorld::AddViewToRender(pReflectionView->m_hView);
    }
  }

  ezDynamicArray<ReflectionView> m_RenderViews;
  ezDynamicArray<ReflectionView> m_FilterViews;

  ezMutex m_Mutex;
  ezIdTable<ezReflectionProbeId, ProbeUpdateInfo> m_UpdateInfo;

  ezDynamicArray<ezReflectionProbeId> m_ActiveDynamicProbes;
  ezUInt64 m_uiWorldHasSkyLight = 0;

  ezDynamicArray<SortedUpdateInfo> m_SortedUpdateInfo;

  ezGALTextureHandle m_hReflectionSpecularTexture;
  ezGALTextureHandle m_hSkyIrradianceTexture;

  ezHybridArray<ezAmbientCube<ezColorLinear16f>, 64> m_SkyIrradianceStorage;
  ezUInt64 m_uiSkyIrradianceChanged = 0;

  ezMeshResourceHandle m_hDebugSphere;
  ezMaterialResourceHandle m_hDebugMaterial;
};

ezReflectionPool::Data* ezReflectionPool::s_pData;

// static
void ezReflectionPool::RegisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld, float fPriority /*= 1.0f*/)
{
  ProbeUpdateInfo updateInfo;
  updateInfo.m_pWorld = pWorld;
  updateInfo.m_fPriority = ezMath::Clamp(fPriority, ezMath::DefaultEpsilon<float>(), 100.0f);

  EZ_LOCK(s_pData->m_Mutex);

  data.m_Id = s_pData->m_UpdateInfo.Insert(std::move(updateInfo));

  s_pData->m_ActiveDynamicProbes.PushBack(data.m_Id);

  ezUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= EZ_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
}

void ezReflectionPool::DeregisterReflectionProbe(ezReflectionProbeData& data, ezWorld* pWorld)
{
  EZ_LOCK(s_pData->m_Mutex);

  s_pData->m_UpdateInfo.Remove(data.m_Id);
  data.m_Id.Invalidate();

  ezUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~EZ_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
}

// static
void ezReflectionPool::ExtractReflectionProbe(
  ezMsgExtractRenderData& msg, const ezReflectionProbeData& data, const ezComponent* pComponent, float fPriority /*= 1.0f*/)
{
  fPriority = ezMath::Clamp(fPriority, ezMath::DefaultEpsilon<float>(), 100.0f);
  ezVec3 vPosition = pComponent->GetOwner()->GetGlobalPosition();

  ProbeUpdateInfo* pUpdateInfo = nullptr;
  if (!s_pData->m_UpdateInfo.TryGetValue(data.m_Id, pUpdateInfo))
    return;

  ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
  bool bExecuteUpdateSteps = false;

  {
    EZ_LOCK(s_pData->m_Mutex);

    pUpdateInfo->m_fPriority = ezMath::Max(pUpdateInfo->m_fPriority, fPriority);

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

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (data.m_bShowDebugInfo)
  {
    const ezGameObject* pOwner = pComponent->GetOwner();

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(pOwner);
    {
      pRenderData->m_GlobalTransform = pOwner->GetGlobalTransform();
      pRenderData->m_GlobalBounds = pOwner->GetGlobalBounds();
      pRenderData->m_hMesh = s_pData->m_hDebugSphere;
      pRenderData->m_hMaterial = s_pData->m_hDebugMaterial;
      pRenderData->m_Color = ezColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = ezRenderComponent::GetUniqueIdForRendering(pComponent, 0);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);

    if (msg.m_OverrideCategory == ezInvalidRenderDataCategory)
    {
      ezStringBuilder sTemp;
      sTemp.Format("Priority: {}\\nIndex in Update Queue: {}", pUpdateInfo->m_fPriority, pUpdateInfo->m_uiIndexInUpdateQueue);

      ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sTemp, vPosition + ezVec3(0, 0, s_fDebugSphereRadius), ezColor::LightPink, 16,
        ezDebugRenderer::HorizontalAlignment::Center, ezDebugRenderer::VerticalAlignment::Bottom);
    }
  }
#endif
}

// static
void ezReflectionPool::SetConstantSkyIrradiance(const ezWorld* pWorld, const ezAmbientCube<ezColor>& skyIrradiance)
{
  ezUInt32 uiWorldIndex = pWorld->GetIndex();
  ezAmbientCube<ezColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
  }
}

void ezReflectionPool::ResetConstantSkyIrradiance(const ezWorld* pWorld)
{
  ezUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != ezAmbientCube<ezColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = ezAmbientCube<ezColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
  }
}

// static
ezUInt32 ezReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
ezGALTextureHandle ezReflectionPool::GetReflectionSpecularTexture()
{
  return s_pData->m_hReflectionSpecularTexture;
}

// static
ezGALTextureHandle ezReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

// static
void ezReflectionPool::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(ezReflectionPool::Data);

  ezRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void ezReflectionPool::OnEngineShutdown()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezReflectionPool::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  if (e.m_Type != ezRenderWorldExtractionEvent::Type::BeginExtraction)
    return;

  EZ_PROFILE_SCOPE("Reflection Pool Update");

  s_pData->CreateSkyIrradianceTexture();

  if (s_pData->m_ActiveDynamicProbes.IsEmpty())
  {
    return;
  }

  s_pData->CreateReflectionViewsAndResources();

  // generate possible update steps for active probes
  {
    s_pData->SortActiveProbes(s_pData->m_ActiveDynamicProbes, e.m_uiFrameCounter);

    s_pData->GenerateUpdateSteps();

    s_pData->m_ActiveDynamicProbes.Clear();
  }
}

// static
void ezReflectionPool::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  EZ_LOCK(s_pData->m_Mutex);

  ezUInt64 uiWorldHasSkyLight = s_pData->m_uiWorldHasSkyLight;
  ezUInt64 uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if (uiWorldHasSkyLight == 0 || uiSkyIrradianceChanged == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("Sky Irradiance Texture Update");
  auto pGALCommandEncoder = pGALPass->BeginCompute();
  EZ_SCOPE_EXIT(pGALPass->EndCompute(pGALCommandEncoder); pDevice->EndPass(pGALPass););

  for (ezUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
  {
    if ((uiWorldHasSkyLight & EZ_BIT(i)) == 0 && (uiSkyIrradianceChanged & EZ_BIT(i)) != 0)
    {
      ezBoundingBoxu32 destBox;
      destBox.m_vMin.Set(0, i, 0);
      destBox.m_vMax.Set(6, i + 1, 1);

      ezGALSystemMemoryDescription memDesc;
      memDesc.m_pData = &skyIrradianceStorage[i].m_Values[0];
      memDesc.m_uiRowPitch = sizeof(ezAmbientCube<ezColorLinear16f>);

      pGALCommandEncoder->UpdateTexture(s_pData->m_hSkyIrradianceTexture, ezGALTextureSubresource(), destBox, memDesc);

      uiSkyIrradianceChanged &= ~EZ_BIT(i);
    }
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
