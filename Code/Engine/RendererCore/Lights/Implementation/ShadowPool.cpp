#include <RendererCore/RendererCorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Common/LightData.h>


// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ShadowPool)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezShadowPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezShadowPool::OnEngineShutdown();
  }
EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_RenderingShadowsShowPoolStats("Rendering.Shadows.ShowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
ezCVarBool cvar_RenderingShadowsVisCascadeBounds("Rendering.Shadows.VisCascadeBounds", false, ezCVarFlags::Default, "Visualizes the bounding volumes of shadow cascades");
#endif

ezCVarFloat cvar_RenderingShadowsScaleMappingExponent("Rendering.Shadows.ScaleMappingExponent", 1.5f, ezCVarFlags::Default, "Determines how fast the shadow map size is reduced with screen space size");

/// NOTE: The default values for these are defined in ezCoreRenderProfileConfig
///       but they can also be overwritten in custom game states at startup.
EZ_RENDERERCORE_DLL ezCVarInt cvar_RenderingShadowsAtlasSize("Rendering.Shadows.AtlasSize", 4096, ezCVarFlags::RequiresDelayedSync, "The size of the shadow atlas texture.");
EZ_RENDERERCORE_DLL ezCVarInt cvar_RenderingShadowsMaxShadowMapSize("Rendering.Shadows.MaxShadowMapSize", 1024, ezCVarFlags::RequiresDelayedSync, "The max shadow map size used.");
EZ_RENDERERCORE_DLL ezCVarInt cvar_RenderingShadowsMinShadowMapSize("Rendering.Shadows.MinShadowMapSize", 64, ezCVarFlags::RequiresDelayedSync, "The min shadow map size used.");

static ezUInt32 s_uiLastConfigModification = 0;
static float s_fMinRelativeShadowMapSize = 0.0f;

struct ShadowView
{
  ezViewHandle m_hView;
  ezCamera m_Camera;
};

struct ShadowData
{
  ezHybridArray<ezViewHandle, 6> m_Views;
  ezUInt32 m_uiType;
  float m_fShadowMapScale;
  float m_fPenumbraSize;
  float m_fSlopeBias;
  float m_fConstantBias;
  float m_fFadeOutStart;
  float m_fMinRange;
  float m_fActualRange;
  ezUInt32 m_uiPackedDataOffset; // in 16 bytes steps
};

struct LightAndRefView
{
  EZ_DECLARE_POD_TYPE();

  const ezLightComponent* m_pLight;
  const ezView* m_pReferenceView;
};

struct SortedShadowData
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiIndex;
  float m_fShadowMapScale;

  EZ_ALWAYS_INLINE bool operator<(const SortedShadowData& other) const
  {
    if (m_fShadowMapScale > other.m_fShadowMapScale) // we want to sort descending (higher scale first)
      return true;

    return m_uiIndex < other.m_uiIndex;
  }
};

static ezDynamicArray<SortedShadowData> s_SortedShadowData;

struct AtlasCell
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE AtlasCell()
    : m_Rect(0, 0, 0, 0)
  {
    m_uiChildIndices[0] = m_uiChildIndices[1] = m_uiChildIndices[2] = m_uiChildIndices[3] = 0xFFFF;
    m_uiDataIndex = ezInvalidIndex;
  }

  EZ_ALWAYS_INLINE bool IsLeaf() const
  {
    return m_uiChildIndices[0] == 0xFFFF && m_uiChildIndices[1] == 0xFFFF && m_uiChildIndices[2] == 0xFFFF && m_uiChildIndices[3] == 0xFFFF;
  }

  ezRectU32 m_Rect;
  ezUInt16 m_uiChildIndices[4];
  ezUInt32 m_uiDataIndex;
};

static ezDeque<AtlasCell> s_AtlasCells;

static AtlasCell* Insert(AtlasCell* pCell, ezUInt32 uiShadowMapSize, ezUInt32 uiDataIndex)
{
  if (!pCell->IsLeaf())
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[i]];
      if (AtlasCell* pNewCell = Insert(pChildCell, uiShadowMapSize, uiDataIndex))
      {
        return pNewCell;
      }
    }

    return nullptr;
  }
  else
  {
    if (pCell->m_uiDataIndex != ezInvalidIndex)
      return nullptr;

    if (pCell->m_Rect.width < uiShadowMapSize || pCell->m_Rect.height < uiShadowMapSize)
      return nullptr;

    if (pCell->m_Rect.width == uiShadowMapSize && pCell->m_Rect.height == uiShadowMapSize)
    {
      pCell->m_uiDataIndex = uiDataIndex;
      return pCell;
    }

    // Split
    ezUInt32 x = pCell->m_Rect.x;
    ezUInt32 y = pCell->m_Rect.y;
    ezUInt32 w = pCell->m_Rect.width / 2;
    ezUInt32 h = pCell->m_Rect.height / 2;

    ezUInt32 uiCellIndex = s_AtlasCells.GetCount();
    s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x + w, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x, y + h, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x + w, y + h, w, h);

    for (ezUInt32 i = 0; i < 4; ++i)
    {
      pCell->m_uiChildIndices[i] = static_cast<ezUInt16>(uiCellIndex + i);
    }

    AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[0]];
    return Insert(pChildCell, uiShadowMapSize, uiDataIndex);
  }
}

static ezUInt8 s_uiWarnCounter = 0;

static ezRectU32 FindAtlasRect(ezUInt32 uiShadowMapSize, ezUInt32 uiDataIndex)
{
  EZ_ASSERT_DEBUG(ezMath::IsPowerOf2(uiShadowMapSize), "Size must be power of 2");

  AtlasCell* pCell = Insert(&s_AtlasCells[0], uiShadowMapSize, uiDataIndex);
  if (pCell != nullptr)
  {
    EZ_ASSERT_DEBUG(pCell->IsLeaf() && pCell->m_uiDataIndex == uiDataIndex, "Implementation error");
    return pCell->m_Rect;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (s_uiWarnCounter == 0)
  {
    ezLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
  }

  ++s_uiWarnCounter; // rely on integer overflow to reach 0 again
#endif

  return ezRectU32(0, 0, 0, 0);
}

static float AddSafeBorder(ezAngle fov, float fPenumbraSize)
{
  float fHalfHeight = ezMath::Tan(fov * 0.5f);
  float fNewFov = ezMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
  return fNewFov;
}

ezTagSet s_ExcludeTagsWhiteList;

static void CopyExcludeTagsOnWhiteList(const ezTagSet& referenceTags, ezTagSet& out_targetTags)
{
  out_targetTags.Clear();
  out_targetTags.SetByName("EditorHidden");

  for (auto& tag : referenceTags)
  {
    if (s_ExcludeTagsWhiteList.IsSet(tag))
    {
      out_targetTags.Set(tag);
    }
  }
}

// must not be in anonymous namespace
template <>
struct ezHashHelper<LightAndRefView>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(LightAndRefView value) { return ezHashingUtils::xxHash32(&value.m_pLight, sizeof(LightAndRefView)); }

  EZ_ALWAYS_INLINE static bool Equal(const LightAndRefView& a, const LightAndRefView& b)
  {
    return a.m_pLight == b.m_pLight && a.m_pReferenceView == b.m_pReferenceView;
  }
};

//////////////////////////////////////////////////////////////////////////

struct ezShadowPool::Data
{
  Data() { Clear(); }

  ~Data()
  {
    for (auto& shadowView : m_ShadowViews)
    {
      ezRenderWorld::DeleteView(shadowView.m_hView);
    }

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    if (!m_hShadowAtlasTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hShadowAtlasTexture);
      m_hShadowAtlasTexture.Invalidate();
    }

    if (!m_hShadowDataBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hShadowDataBuffer);
      m_hShadowDataBuffer.Invalidate();
    }
  }

  enum
  {
    MAX_SHADOW_DATA = 1024
  };

  void CreateShadowAtlasTexture()
  {
    if (m_hShadowAtlasTexture.IsInvalidated())
    {
      // use the current CVar values to initialize the values
      ezUInt32 uiAtlas = cvar_RenderingShadowsAtlasSize;
      ezUInt32 uiMax = cvar_RenderingShadowsMaxShadowMapSize;
      ezUInt32 uiMin = cvar_RenderingShadowsMinShadowMapSize;

      // if the platform profile has changed, use it to reset the defaults
      if (s_uiLastConfigModification != ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter())
      {
        s_uiLastConfigModification = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter();

        const auto* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezCoreRenderProfileConfig>();

        uiAtlas = pConfig->m_uiShadowAtlasTextureSize;
        uiMax = pConfig->m_uiMaxShadowMapSize;
        uiMin = pConfig->m_uiMinShadowMapSize;
      }

      // if the CVars were modified recently (e.g. during game startup), use those values to override the default
      if (cvar_RenderingShadowsAtlasSize.HasDelayedSyncValueChanged())
        uiAtlas = cvar_RenderingShadowsAtlasSize.GetValue(ezCVarValue::DelayedSync);

      if (cvar_RenderingShadowsMaxShadowMapSize.HasDelayedSyncValueChanged())
        uiMax = cvar_RenderingShadowsMaxShadowMapSize.GetValue(ezCVarValue::DelayedSync);

      if (cvar_RenderingShadowsMinShadowMapSize.HasDelayedSyncValueChanged())
        uiMin = cvar_RenderingShadowsMinShadowMapSize.GetValue(ezCVarValue::DelayedSync);

      // make sure the values are valid
      uiAtlas = ezMath::Clamp(ezMath::PowerOfTwo_Floor(uiAtlas), 512u, 8192u);
      uiMax = ezMath::Clamp(ezMath::PowerOfTwo_Floor(uiMax), 64u, 2048u);
      uiMin = ezMath::Clamp(ezMath::PowerOfTwo_Floor(uiMin), 8u, 512u);

      uiMax = ezMath::Min(uiMax, uiAtlas);
      uiMin = ezMath::Min(uiMin, uiMax);

      // write back the clamped values, so that everyone sees the valid values
      cvar_RenderingShadowsAtlasSize = uiAtlas;
      cvar_RenderingShadowsMaxShadowMapSize = uiMax;
      cvar_RenderingShadowsMinShadowMapSize = uiMin;

      // apply the new values
      cvar_RenderingShadowsAtlasSize.SetToDelayedSyncValue();
      cvar_RenderingShadowsMaxShadowMapSize.SetToDelayedSyncValue();
      cvar_RenderingShadowsMinShadowMapSize.SetToDelayedSyncValue();

      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(cvar_RenderingShadowsAtlasSize, cvar_RenderingShadowsAtlasSize, ezGALResourceFormat::D16);

      m_hShadowAtlasTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(desc);

      s_fMinRelativeShadowMapSize = (cvar_RenderingShadowsMinShadowMapSize - 1.0f) / cvar_RenderingShadowsMaxShadowMapSize;
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hShadowDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  ezViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();
    CreateShadowDataBuffer();

    ezView* pView = nullptr;
    ezViewHandle hView = ezRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(ezCameraUsageHint::Shadow);

    ezGALRenderTargets renderTargets;
    renderTargets.m_hDSTarget = m_hShadowAtlasTexture;
    pView->SetRenderTargets(renderTargets);

    EZ_ASSERT_DEV(m_ShadowViewsMutex.IsLocked(), "m_ShadowViewsMutex must be locked at this point.");
    m_ShadowViewsMutex.Unlock(); // if the resource gets loaded in the call below, his could lead to a deadlock

    // ShadowMapRenderPipeline.ezRenderPipelineAsset
    pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

    m_ShadowViewsMutex.Lock();

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnEndExtraction before
    // rendering.
    pView->SetViewport(ezRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    pView->m_IncludeTags.Set(tagCastShadows);

    pView->m_ExcludeTags.SetByName("EditorHidden");

    return hView;
  }

  ShadowView& GetShadowView(ezView*& out_pView)
  {
    EZ_LOCK(m_ShadowViewsMutex);

    if (m_uiUsedViews == m_ShadowViews.GetCount())
    {
      m_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = m_ShadowViews[m_uiUsedViews];
    if (ezRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
      out_pView->SetLodCamera(nullptr);
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(const ezLightComponent* pLight, const ezView* pReferenceView, float fShadowMapScale, ezUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
  {
    EZ_LOCK(m_ShadowDataMutex);

    LightAndRefView key = {pLight, pReferenceView};

    ezUInt32 uiDataIndex = ezInvalidIndex;
    if (m_LightToShadowDataTable.TryGetValue(key, uiDataIndex))
    {
      out_pData = &m_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = ezMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    m_ShadowData.EnsureCount(m_uiUsedShadowData + 1);

    out_pData = &m_ShadowData[m_uiUsedShadowData];
    out_pData->m_fShadowMapScale = fShadowMapScale;
    out_pData->m_fPenumbraSize = pLight->GetPenumbraSize();
    out_pData->m_fSlopeBias = pLight->GetSlopeBias() * 100.0f;       // map from user friendly range to real range
    out_pData->m_fConstantBias = pLight->GetConstantBias() / 100.0f; // map from user friendly range to real range
    out_pData->m_fFadeOutStart = 1.0f;
    out_pData->m_fMinRange = 1.0f;
    out_pData->m_fActualRange = 1.0f;
    out_pData->m_uiPackedDataOffset = m_uiUsedPackedShadowData;

    m_LightToShadowDataTable.Insert(key, m_uiUsedShadowData);

    ++m_uiUsedShadowData;
    m_uiUsedPackedShadowData += uiPackedDataSizeInBytes / sizeof(ezVec4);

    return false;
  }

  void Clear()
  {
    m_uiUsedViews = 0;
    m_uiUsedShadowData = 0;

    m_LightToShadowDataTable.Clear();

    m_uiUsedPackedShadowData = 0;
  }

  ezMutex m_ShadowViewsMutex;
  ezDeque<ShadowView> m_ShadowViews;
  ezUInt32 m_uiUsedViews = 0;

  ezMutex m_ShadowDataMutex;
  ezDeque<ShadowData> m_ShadowData;
  ezUInt32 m_uiUsedShadowData = 0;
  ezHashTable<LightAndRefView, ezUInt32> m_LightToShadowDataTable;

  ezDynamicArray<ezVec4, ezAlignedAllocatorWrapper> m_PackedShadowData[2];
  ezUInt32 m_uiUsedPackedShadowData = 0; // in 16 bytes steps (sizeof(ezVec4))

  ezGALTextureHandle m_hShadowAtlasTexture;
  ezGALBufferHandle m_hShadowDataBuffer;
};

//////////////////////////////////////////////////////////////////////////

ezShadowPool::Data* ezShadowPool::s_pData = nullptr;

// static
ezUInt32 ezShadowPool::AddDirectionalLight(const ezDirectionalLightComponent* pDirLight, const ezView* pReferenceView)
{
  EZ_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  // No shadows in orthographic views
  if (pReferenceView->GetCullingCamera()->IsOrthographic())
  {
    return ezInvalidIndex;
  }

  float fMaxReferenceSize = ezMath::Max(pReferenceView->GetViewport().width, pReferenceView->GetViewport().height);
  float fShadowMapScale = fMaxReferenceSize / cvar_RenderingShadowsMaxShadowMapSize;

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pDirLight, pReferenceView, fShadowMapScale, sizeof(ezDirShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  ezUInt32 uiNumCascades = ezMath::Min(pDirLight->GetNumCascades(), 4u);
  const ezCamera* pReferenceCamera = pReferenceView->GetCullingCamera();

  pData->m_uiType = LIGHT_TYPE_DIR;
  pData->m_fFadeOutStart = pDirLight->GetFadeOutStart();
  pData->m_fMinRange = pDirLight->GetMinShadowRange();
  pData->m_Views.SetCount(uiNumCascades);

  // determine cascade ranges
  float fNearPlane = pReferenceCamera->GetNearPlane();
  float fShadowRange = pDirLight->GetMinShadowRange();
  float fSplitModeWeight = pDirLight->GetSplitModeWeight();

  float fCascadeRanges[4];
  for (ezUInt32 i = 0; i < uiNumCascades; ++i)
  {
    float f = float(i + 1) / uiNumCascades;
    float logDistance = fNearPlane * ezMath::Pow(fShadowRange / fNearPlane, f);
    float linearDistance = fNearPlane + (fShadowRange - fNearPlane) * f;
    fCascadeRanges[i] = ezMath::Lerp(linearDistance, logDistance, fSplitModeWeight);
  }

  const ezStringView viewNames[4] = {"-C0"_ezsv, "-C1"_ezsv, "-C2"_ezsv, "-C3"_ezsv};
  ezStringBuilder tmp;

  const ezGameObject* pOwner = pDirLight->GetOwner();
  const ezVec3 vLightDirForwards = pOwner->GetGlobalDirForwards();
  const ezVec3 vLightDirUp = pOwner->GetGlobalDirUp();

  float fAspectRatio = pReferenceView->GetViewport().width / pReferenceView->GetViewport().height;

  float fCascadeStart = 0.0f;
  float fCascadeEnd = 0.0f;
  float fTanFovX = ezMath::Tan(pReferenceCamera->GetFovX(fAspectRatio) * 0.5f);
  float fTanFovY = ezMath::Tan(pReferenceCamera->GetFovY(fAspectRatio) * 0.5f);
  ezVec3 corner = ezVec3(fTanFovX, fTanFovY, 1.0f);

  float fNearPlaneOffset = pDirLight->GetNearPlaneOffset();

  for (ezUInt32 i = 0; i < uiNumCascades; ++i)
  {
    ezView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      if (pOwner->GetName().IsEmpty())
      {
        tmp.Set("DirLight", viewNames[i]);
      }
      else
      {
        tmp.Set(pOwner->GetName(), viewNames[i]);
      }

      pView->SetName(tmp);

      pView->SetWorld(const_cast<ezWorld*>(pDirLight->GetWorld()));
      pView->SetLodCamera(pReferenceCamera);
      pView->SetRenderPassProperty("ShadowDepth", "RenderTransparentObjects", pDirLight->GetTransparentShadows());
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;

      // Make sure that the last cascade always covers the whole range so effects like e.g. particles can get away with
      // sampling only the last cascade.
      if (i == uiNumCascades - 1)
      {
        fCascadeStart = 0.0f;
      }

      fCascadeEnd = fCascadeRanges[i];

      ezVec3 startCorner = corner * fCascadeStart;
      ezVec3 endCorner = corner * fCascadeEnd;
      pData->m_fActualRange = endCorner.GetLength();

      // Find the enclosing sphere for the frustum:
      // The sphere center must be on the view's center ray and should be equally far away from the corner points.
      // x = distance from camera origin to sphere center
      // d1^2 = sc.x^2 + sc.y^2 + (x - sc.z)^2
      // d2^2 = ec.x^2 + ec.y^2 + (x - ec.z)^2
      // d1 == d2 and solve for x:
      float x = (endCorner.Dot(endCorner) - startCorner.Dot(startCorner)) / (2.0f * (endCorner.z - startCorner.z));
      x = ezMath::Min(x, fCascadeEnd);

      ezVec3 center = pReferenceCamera->GetPosition() + pReferenceCamera->GetDirForwards() * x;

      // prevent too large values
      // sometimes this can happen when imported data is badly scaled and thus way too large
      // then adding dirForwards result in no change and we run into other asserts later
      center.x = ezMath::Clamp(center.x, -1000000.0f, +1000000.0f);
      center.y = ezMath::Clamp(center.y, -1000000.0f, +1000000.0f);
      center.z = ezMath::Clamp(center.z, -1000000.0f, +1000000.0f);

      endCorner.z -= x;
      const float radius = endCorner.GetLength();

      const float fCameraToCenterDistance = radius + fNearPlaneOffset;
      const ezVec3 shadowCameraPos = center - vLightDirForwards * fCameraToCenterDistance;
      const float fFarPlane = radius + fCameraToCenterDistance;

      ezCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vLightDirUp);
      camera.SetCameraMode(ezCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      const ezMat4 worldToLightMatrix = pView->GetViewMatrix(ezCameraEye::Left);
      const float texelInWorld = (2.0f * radius) / cvar_RenderingShadowsMaxShadowMapSize;
      ezVec3 offset = worldToLightMatrix.TransformPosition(ezVec3::MakeZero());
      offset.x -= ezMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= ezMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsVisCascadeBounds)
      {
        ezDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), ezBoundingSphere::MakeFromCenterAndRadius(center, radius), ezColorScheme::LightUI(ezColorScheme::Orange));

        ezDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(0, 0, fCameraToCenterDistance), radius), ezColorScheme::LightUI(ezColorScheme::Red), ezTransform::MakeFromMat4(camera.GetViewMatrix().GetInverse()));
      }
#endif
    }

    ezRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
ezUInt32 ezShadowPool::AddPointLight(const ezPointLightComponent* pPointLight, float fScreenSpaceSize, const ezView* pReferenceView)
{
  EZ_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  const float fShadowMapScale = fScreenSpaceSize * 0.5f;
  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fShadowMapScale, sizeof(ezPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  ezVec3 faceDirs[6] = {
    ezVec3(1.0f, 0.0f, 0.0f),
    ezVec3(-1.0f, 0.0f, 0.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, -1.0f),
  };

  const ezStringView viewNames[6] = {
    "+X"_ezsv,
    "-X"_ezsv,
    "+Y"_ezsv,
    "-Y"_ezsv,
    "+Z"_ezsv,
    "-Z"_ezsv,
  };

  const ezGameObject* pOwner = pPointLight->GetOwner();
  ezVec3 vPosition = pOwner->GetGlobalPosition();
  ezVec3 vUp = ezVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = ezMath::Max(pPointLight->GetPenumbraSize(), (0.5f / cvar_RenderingShadowsMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(ezAngle::MakeFromDegree(90.0f), fPenumbraSize);

  ///\todo expose somewhere
  float fNearPlane = 0.1f;
  float fFarPlane = pPointLight->GetEffectiveRange();

  ezStringBuilder tmp;

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;


    // Setup view
    {
      if (pOwner->GetName().IsEmpty())
      {
        tmp.Set("PointLight", viewNames[i]);
      }
      else
      {
        tmp.Set(pOwner->GetName(), viewNames[i]);
      }

      pView->SetName(tmp);

      pView->SetWorld(const_cast<ezWorld*>(pPointLight->GetWorld()));
      pView->SetRenderPassProperty("ShadowDepth", "RenderTransparentObjects", pPointLight->GetTransparentShadows());
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      ezVec3 vForward = faceDirs[i];

      ezCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    ezRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
ezUInt32 ezShadowPool::AddSpotLight(const ezSpotLightComponent* pSpotLight, float fScreenSpaceSize, const ezView* pReferenceView)
{
  EZ_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  const float fShadowMapScale = fScreenSpaceSize * 0.5f;
  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fShadowMapScale, sizeof(ezSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  ezView* pView = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0] = shadowView.m_hView;

  const ezGameObject* pOwner = pSpotLight->GetOwner();

  // Setup view
  {
    if (pOwner->GetName().IsEmpty())
    {
      pView->SetName("SpotLight"_ezsv);
    }
    else
    {
      pView->SetName(pOwner->GetName());
    }

    pView->SetWorld(const_cast<ezWorld*>(pSpotLight->GetWorld()));
    pView->SetRenderPassProperty("ShadowDepth", "RenderTransparentObjects", pSpotLight->GetTransparentShadows());
    CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
  }

  // Setup camera
  {
    const ezGameObject* pOwner = pSpotLight->GetOwner();
    ezVec3 vPosition = pOwner->GetGlobalPosition();
    ezVec3 vForward = pOwner->GetGlobalDirForwards();
    ezVec3 vUp = pOwner->GetGlobalDirUp();

    float fFov = AddSafeBorder(pSpotLight->GetOuterSpotAngle(), pSpotLight->GetPenumbraSize());
    float fNearPlane = 0.1f; ///\todo expose somewhere
    float fFarPlane = pSpotLight->GetEffectiveRange();

    ezCamera& camera = shadowView.m_Camera;
    camera.LookAt(vPosition, vPosition + vForward, vUp);
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
  }

  ezRenderWorld::AddViewToRender(shadowView.m_hView);

  return pData->m_uiPackedDataOffset;
}

// static
ezGALTextureHandle ezShadowPool::GetShadowAtlasTexture()
{
  return s_pData->m_hShadowAtlasTexture;
}

// static
ezGALBufferHandle ezShadowPool::GetShadowDataBuffer()
{
  return s_pData->m_hShadowDataBuffer;
}

// static
void ezShadowPool::AddExcludeTagToWhiteList(const ezTag& tag)
{
  s_ExcludeTagsWhiteList.Set(tag);
}

// static
void ezShadowPool::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(ezShadowPool::Data);

  ezRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void ezShadowPool::OnEngineShutdown()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezShadowPool::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  if (e.m_Type != ezRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  EZ_PROFILE_SCOPE("Shadow Pool Update");

  ezUInt32 uiDataIndex = ezRenderWorld::GetDataIndexForExtraction();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  packedShadowData.SetCountUninitialized(s_pData->m_uiUsedPackedShadowData);

  if (s_pData->m_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (ezUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_pData->m_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    auto& sorted = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_uiType == LIGHT_TYPE_DIR ? 100.0f : ezMath::Min(shadowData.m_fShadowMapScale, 10.0f);
  }

  s_SortedShadowData.Sort();

  // Prepare atlas
  s_AtlasCells.Clear();
  s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(0, 0, cvar_RenderingShadowsAtlasSize, cvar_RenderingShadowsAtlasSize);

  float fAtlasInvWidth = 1.0f / cvar_RenderingShadowsAtlasSize;
  float fAtlasInvHeight = 1.0f / cvar_RenderingShadowsAtlasSize;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezUInt32 uiTotalAtlasSize = cvar_RenderingShadowsAtlasSize * cvar_RenderingShadowsAtlasSize;
  ezUInt32 uiUsedAtlasSize = 0;

  ezDebugRendererContext debugContext(ezWorld::GetWorld(0));
  if (const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView))
  {
    debugContext = ezDebugRendererContext(pView->GetHandle());
  }

  if (cvar_RenderingShadowsShowPoolStats)
  {
    ezDebugRenderer::DrawInfoText(debugContext, ezDebugTextPlacement::TopLeft, "0ShadowPoolStats", "Shadow Pool Stats:", ezColor::LightSteelBlue);
    ezDebugRenderer::DrawInfoText(debugContext, ezDebugTextPlacement::TopLeft, "1ShadowPoolStats", "Details (Name: Size - Atlas Offset)", ezColor::LightSteelBlue);
  }

#endif

  for (auto& sorted : s_SortedShadowData)
  {
    ezUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    ezUInt32 uiMaxShadowMapSize = cvar_RenderingShadowsMaxShadowMapSize;
    float fMinRelativeShadowMapSize = s_fMinRelativeShadowMapSize;

    // point lights use a lot of atlas space thus we cut the shadow map size in half
    if (shadowData.m_uiType == LIGHT_TYPE_POINT)
    {
      uiMaxShadowMapSize /= 2;
      fMinRelativeShadowMapSize *= 2.0f;
    }

    const float fClampedShadowMapScale = ezMath::Clamp(ezMath::Pow(shadowData.m_fShadowMapScale, cvar_RenderingShadowsScaleMappingExponent), fMinRelativeShadowMapSize, 1.0f);
    const ezUInt32 uiShadowMapSize = ezMath::PowerOfTwo_Ceil((ezUInt32)(uiMaxShadowMapSize * fClampedShadowMapScale));

    ezHybridArray<ezView*, 8> shadowViews;
    ezHybridArray<ezRectU32, 8> atlasRects;

    // Fill atlas
    for (ezUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      ezView* pShadowView = nullptr;
      ezRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView);
      shadowViews.PushBack(pShadowView);

      EZ_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

      ezRectU32 atlasRect = FindAtlasRect(uiShadowMapSize, uiShadowDataIndex);
      atlasRects.PushBack(atlasRect);

      pShadowView->SetViewport(ezRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsShowPoolStats)
      {
        ezDebugRenderer::DrawInfoText(debugContext, ezDebugTextPlacement::TopLeft, "1ShadowPoolStats", ezFmt("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y), ezColor::LightSteelBlue);

        uiUsedAtlasSize += atlasRect.width * atlasRect.height;
      }
#endif
    }

    // Fill shadow data
    if (shadowData.m_uiType == LIGHT_TYPE_DIR)
    {
      ezUInt32 uiNumCascades = shadowData.m_Views.GetCount();

      ezUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, 0);
      ezMat4& worldToLightMatrix = *reinterpret_cast<ezMat4*>(&packedShadowData[uiMatrixIndex]);

      worldToLightMatrix = shadowViews[0]->GetViewProjectionMatrix(ezCameraEye::Left);

      for (ezUInt32 uiViewIndex = 0; uiViewIndex < uiNumCascades; ++uiViewIndex)
      {
        if (uiViewIndex >= 1)
        {
          ezMat4 cascadeToWorldMatrix = shadowViews[uiViewIndex]->GetInverseViewProjectionMatrix(ezCameraEye::Left);
          ezVec3 cascadeCorner = cascadeToWorldMatrix.TransformPosition(ezVec3(0.0f));
          cascadeCorner = worldToLightMatrix.TransformPosition(cascadeCorner);

          ezVec3 otherCorner = cascadeToWorldMatrix.TransformPosition(ezVec3(1.0f));
          otherCorner = worldToLightMatrix.TransformPosition(otherCorner);

          ezUInt32 uiCascadeScaleIndex = GET_CASCADE_SCALE_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);
          ezUInt32 uiCascadeOffsetIndex = GET_CASCADE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);

          ezVec4& cascadeScale = packedShadowData[uiCascadeScaleIndex];
          ezVec4& cascadeOffset = packedShadowData[uiCascadeOffsetIndex];

          cascadeScale = ezVec3(1.0f).CompDiv(otherCorner - cascadeCorner).GetAsVec4(1.0f);
          cascadeOffset = cascadeCorner.GetAsVec4(0.0f).CompMul(-cascadeScale);
        }

        ezUInt32 uiAtlasScaleOffsetIndex = GET_ATLAS_SCALE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        ezVec4& atlasScaleOffset = packedShadowData[uiAtlasScaleOffsetIndex];

        ezRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          ezVec2 scale = ezVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          ezVec2 offset = ezVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          // combine with tex scale offset
          atlasScaleOffset.x = scale.x * 0.5f;
          atlasScaleOffset.y = scale.y * -0.5f;
          atlasScaleOffset.z = offset.x + scale.x * 0.5f;
          atlasScaleOffset.w = offset.y + scale.y * 0.5f;
        }
        else
        {
          atlasScaleOffset.Set(1.0f, 1.0f, 0.0f, 0.0f);
        }
      }

      const ezCamera* pFirstCascadeCamera = shadowViews[0]->GetCamera();
      const ezCamera* pLastCascadeCamera = shadowViews[uiNumCascades - 1]->GetCamera();

      float cascadeSize = pFirstCascadeCamera->GetFovOrDim();
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = ezMath::Max(shadowData.m_fPenumbraSize / cascadeSize, texelSize);
      float goodPenumbraSize = 8.0f / uiShadowMapSize;
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // params
      {
        // tweak values to keep the default values consistent with spot and point lights
        float slopeBias = shadowData.m_fSlopeBias * ezMath::Max(penumbraSize, goodPenumbraSize);
        float constantBias = shadowData.m_fConstantBias * 0.2f;
        ezUInt32 uilastCascadeIndex = uiNumCascades - 1;

        ezUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        ezVec4& shadowParams = packedShadowData[uiParamsIndex];
        shadowParams.x = slopeBias;
        shadowParams.y = constantBias;
        shadowParams.z = penumbraSize * relativeShadowSize;
        shadowParams.w = *reinterpret_cast<float*>(&uilastCascadeIndex);
      }

      // params2
      {
        float ditherMultiplier = 0.2f / cascadeSize;
        float zRange = cascadeSize / pFirstCascadeCamera->GetFarPlane();

        float actualPenumbraSize = shadowData.m_fPenumbraSize / pLastCascadeCamera->GetFovOrDim();
        float penumbraSizeIncrement = ezMath::Max(goodPenumbraSize - actualPenumbraSize, 0.0f) / shadowData.m_fMinRange;

        ezUInt32 uiParams2Index = GET_SHADOW_PARAMS2_INDEX(shadowData.m_uiPackedDataOffset);
        ezVec4& shadowParams2 = packedShadowData[uiParams2Index];
        shadowParams2.x = 1.0f - ezMath::Max(penumbraSize, goodPenumbraSize);
        shadowParams2.y = ditherMultiplier;
        shadowParams2.z = ditherMultiplier * zRange;
        shadowParams2.w = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale = -1.0f / fadeOutRange;
        float xyOffset = -xyScale;
        ezUInt32 xyScaleOffset = ezShaderUtils::PackFloat16intoUint(xyScale, xyOffset);

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale = -1.0f / zFadeOutRange;
        float zOffset = -zScale;
        ezUInt32 zScaleOffset = ezShaderUtils::PackFloat16intoUint(zScale, zOffset);

        float distanceFadeOutRange = fadeOutRange * shadowData.m_fActualRange;
        float distanceScale = -1.0f / distanceFadeOutRange;
        float distanceOffset = -distanceScale * shadowData.m_fActualRange;

        ezUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        ezVec4& fadeOutParams = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x = *reinterpret_cast<float*>(&xyScaleOffset);
        fadeOutParams.y = *reinterpret_cast<float*>(&zScaleOffset);
        fadeOutParams.z = distanceScale;
        fadeOutParams.w = distanceOffset;
      }
    }
    else // spot or point light
    {
      ezMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(ezVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(ezVec3(0.5f, 0.5f, 0.0f));

      ezAngle fov = ezAngle::MakeZero();
      float fRange = 0.0f;

      for (ezUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
      {
        ezView* pShadowView = shadowViews[uiViewIndex];
        EZ_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

        ezUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        ezMat4& worldToLightMatrix = *reinterpret_cast<ezMat4*>(&packedShadowData[uiMatrixIndex]);

        ezRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          ezVec2 scale = ezVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          ezVec2 offset = ezVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          ezMat4 atlasMatrix;
          atlasMatrix.SetIdentity();
          atlasMatrix.SetDiagonal(ezVec4(scale.x, scale.y, 1.0f, 1.0f));
          atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

          fov = pShadowView->GetCamera()->GetFovY(1.0f);
          fRange = pShadowView->GetCamera()->GetFarPlane();
          const ezMat4& viewProjection = pShadowView->GetViewProjectionMatrix(ezCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      const float screenHeight = ezMath::Tan(fov * 0.5f) * 20.0f; // screen height in world-space at 10m distance
      const float texelSize = 1.0f / uiShadowMapSize;
      const float penumbraSize = ezMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      const float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // empirical tweak factors
      const float fovFactor = 0.15f * ezMath::Pow(5.5f, fov.GetRadian());
      const float rangeFactor = ezMath::Max(0.018f * fRange + 0.0098f * fRange * fRange, 0.1f);
      const float slopeBias = shadowData.m_fSlopeBias * penumbraSize * fovFactor * rangeFactor;
      const float constantBias = shadowData.m_fConstantBias * cvar_RenderingShadowsMaxShadowMapSize / uiShadowMapSize;

      ezUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      ezVec4& shadowParams = packedShadowData[uiParamsIndex];
      shadowParams.x = slopeBias;
      shadowParams.y = constantBias;
      shadowParams.z = penumbraSize * relativeShadowSize;
      shadowParams.w = 0.0f;
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingShadowsShowPoolStats)
  {
    const float fUtil = (float)uiUsedAtlasSize / uiTotalAtlasSize;

    ezDebugRenderer::DrawInfoText(debugContext, ezDebugTextPlacement::TopLeft, "0ShadowPoolStats", ezFmt("Atlas Utilization: {0}%%", ezArgF(100.0f * fUtil, 2)), ezMath::Lerp(ezColor::LimeGreen, ezColor::Red, fUtil));
  }
#endif

  s_pData->Clear();
}

// static
void ezShadowPool::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hShadowAtlasTexture.IsInvalidated() || s_pData->m_hShadowDataBuffer.IsInvalidated())
    return;

  if (cvar_RenderingShadowsAtlasSize.HasDelayedSyncValueChanged() || cvar_RenderingShadowsMinShadowMapSize.HasDelayedSyncValueChanged() || cvar_RenderingShadowsMaxShadowMapSize.HasDelayedSyncValueChanged() || s_uiLastConfigModification != ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetLastModificationCounter())
  {
    OnEngineShutdown();
    OnEngineStartup();
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pCommandEncoder = pDevice->BeginCommands("Shadow Atlas");

  ezGALRenderingSetup renderingSetup;
  renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(s_pData->m_hShadowAtlasTexture));
  renderingSetup.SetClearDepth();

  pCommandEncoder->BeginRendering(renderingSetup);

  ezUInt32 uiDataIndex = ezRenderWorld::GetDataIndexForRendering();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  if (!packedShadowData.IsEmpty())
  {
    EZ_PROFILE_SCOPE("Shadow Data Buffer Update");

    pCommandEncoder->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr(), ezGALUpdateMode::AheadOfTime);
  }

  pCommandEncoder->EndRendering();
  pDevice->EndCommands(pCommandEncoder);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ShadowPool);
