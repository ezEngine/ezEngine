#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

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
ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

namespace
{
  static ezUInt32 s_uiShadowAtlasTextureWidth = 4096; ///\todo make this configurable
  static ezUInt32 s_uiShadowAtlasTextureHeight = 4096;
  static ezUInt32 s_uiShadowMapSize = 1024;
  static ezUInt32 s_uiMinShadowMapSize = 64;
  static float s_fFadeOutScaleStart = (s_uiMinShadowMapSize + 1.0f) / s_uiShadowMapSize;
  static float s_fFadeOutScaleEnd = s_fFadeOutScaleStart * 0.5f;

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
        pCell->m_uiChildIndices[i] = uiCellIndex + i;
      }

      AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[0]];
      return Insert(pChildCell, uiShadowMapSize, uiDataIndex);
    }
  }

  static ezRectU32 FindAtlasRect(ezUInt32 uiShadowMapSize, ezUInt32 uiDataIndex)
  {
    EZ_ASSERT_DEBUG(ezMath::IsPowerOf2(uiShadowMapSize), "Size must be power of 2");

    AtlasCell* pCell = Insert(&s_AtlasCells[0], uiShadowMapSize, uiDataIndex);
    if (pCell != nullptr)
    {
      EZ_ASSERT_DEBUG(pCell->IsLeaf() && pCell->m_uiDataIndex == uiDataIndex, "Implementation error");
      return pCell->m_Rect;
    }

    ezLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
    return ezRectU32(0, 0, 0, 0);
  }

  static float AddSafeBorder(ezAngle fov, float fPenumbraSize)
  {
    float fHalfHeight = ezMath::Tan(fov * 0.5f);
    float fNewFov = ezMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
    return fNewFov;
  }
} // namespace

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
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight, ezGALResourceFormat::D16);

      m_hShadowAtlasTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferType = ezGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
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

    ezGALRenderTargetSetup renderTargetSetup;
    renderTargetSetup.SetDepthStencilTarget(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hShadowAtlasTexture));
    pView->SetRenderTargetSetup(renderTargetSetup);

    // ShadowMapRenderPipeline.ezRenderPipelineAsset
    pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

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
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(
    const ezLightComponent* pLight, const ezView* pReferenceView, float fShadowMapScale, ezUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
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
  float fShadowMapScale = fMaxReferenceSize / s_uiShadowMapSize;

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

  const char* viewNames[4] = {"DirLightViewC0", "DirLightViewC1", "DirLightViewC2", "DirLightViewC3"};

  const ezGameObject* pOwner = pDirLight->GetOwner();
  ezVec3 vForward = pOwner->GetGlobalDirForwards();
  ezVec3 vUp = pOwner->GetGlobalDirUp();

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
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<ezWorld*>(pDirLight->GetWorld()));
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;
      fCascadeEnd = fCascadeRanges[i];

      ezVec3 startCorner = corner * fCascadeStart;
      ezVec3 endCorner = corner * fCascadeEnd;

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
      float radius = endCorner.GetLength();

      if (false)
      {
        ezDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), ezBoundingSphere(center, radius), ezColor::OrangeRed);
      }

      float fCameraToCenterDistance = radius + fNearPlaneOffset;
      ezVec3 shadowCameraPos = center - vForward * fCameraToCenterDistance;
      float fFarPlane = radius + fCameraToCenterDistance;

      ezCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vUp);
      camera.SetCameraMode(ezCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      ezMat4 worldToLightMatrix = pView->GetViewMatrix(ezCameraEye::Left);
      ezVec3 offset = worldToLightMatrix.TransformPosition(ezVec3::ZeroVector());
      float texelInWorld = (2.0f * radius) / s_uiShadowMapSize;
      offset.x -= ezMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= ezMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);
    }

    ezRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
ezUInt32 ezShadowPool::AddPointLight(const ezPointLightComponent* pPointLight, float fScreenSpaceSize)
{
  EZ_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd * 2.0f)
  {
    return ezInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fScreenSpaceSize, sizeof(ezPointShadowData), pData))
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

  const char* viewNames[6] = {
    "PointLightView+X",
    "PointLightView-X",
    "PointLightView+Y",
    "PointLightView-Y",
    "PointLightView+Z",
    "PointLightView-Z",
  };

  const ezGameObject* pOwner = pPointLight->GetOwner();
  ezVec3 vPosition = pOwner->GetGlobalPosition();
  ezVec3 vUp = ezVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = ezMath::Max(pPointLight->GetPenumbraSize(), (0.5f / s_uiMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(ezAngle::Degree(90.0f), fPenumbraSize);

  float fNearPlane = 0.1f; ///\todo expose somewhere
  float fFarPlane = pPointLight->GetEffectiveRange();

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<ezWorld*>(pPointLight->GetWorld()));
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
ezUInt32 ezShadowPool::AddSpotLight(const ezSpotLightComponent* pSpotLight, float fScreenSpaceSize)
{
  EZ_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd)
  {
    return ezInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fScreenSpaceSize, sizeof(ezSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  ezView* pView = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0] = shadowView.m_hView;

  // Setup view
  {
    pView->SetName("SpotLightView");
    pView->SetWorld(const_cast<ezWorld*>(pSpotLight->GetWorld()));
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
  s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(0, 0, s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight);

  float fAtlasInvWidth = 1.0f / s_uiShadowAtlasTextureWidth;
  float fAtlasInvHeight = 1.0f / s_uiShadowAtlasTextureWidth;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezUInt32 uiTotalAtlasSize = s_uiShadowAtlasTextureWidth * s_uiShadowAtlasTextureHeight;
  ezUInt32 uiUsedAtlasSize = 0;

  ezDebugRendererContext debugContext(ezWorld::GetWorld(0));

  if (CVarShadowPoolStats)
  {
    ezDebugRenderer::Draw2DText(debugContext, "Shadow Pool Stats", ezVec2I32(10, 200), ezColor::LightSteelBlue);
    ezDebugRenderer::Draw2DText(debugContext, "Details (Name: Size - Atlas Offset)", ezVec2I32(10, 250), ezColor::LightSteelBlue);
  }

  ezInt32 iCurrentStatsOffset = 270;
#endif

  for (auto& sorted : s_SortedShadowData)
  {
    ezUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    ezUInt32 uiShadowMapSize = s_uiShadowMapSize;
    float fadeOutStart = s_fFadeOutScaleStart;
    float fadeOutEnd = s_fFadeOutScaleEnd;

    // point lights use a lot of atlas space thus we cut the shadow map size in half
    if (shadowData.m_uiType == LIGHT_TYPE_POINT)
    {
      uiShadowMapSize /= 2;
      fadeOutStart *= 2.0f;
      fadeOutEnd *= 2.0f;
    }

    uiShadowMapSize = ezMath::PowerOfTwo_Ceil((ezUInt32)(uiShadowMapSize * ezMath::Clamp(shadowData.m_fShadowMapScale, fadeOutStart, 1.0f)));

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
      if (CVarShadowPoolStats)
      {
        ezStringBuilder sb;
        sb.Format("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y);

        ezDebugRenderer::Draw2DText(debugContext, sb, ezVec2I32(10, iCurrentStatsOffset), ezColor::LightSteelBlue);
        iCurrentStatsOffset += 20;

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
        shadowParams2.x = 1.0f - (ezMath::Max(penumbraSize, goodPenumbraSize) + texelSize) * 2.0f;
        shadowParams2.y = ditherMultiplier;
        shadowParams2.z = ditherMultiplier * zRange;
        shadowParams2.w = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale = -1.0f / fadeOutRange;
        float xyOffset = -xyScale;

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale = -1.0f / zFadeOutRange;
        float zOffset = -zScale;

        ezUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        ezVec4& fadeOutParams = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x = xyScale;
        fadeOutParams.y = xyOffset;
        fadeOutParams.z = zScale;
        fadeOutParams.w = zOffset;
      }
    }
    else // spot or point light
    {
      ezMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(ezVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(ezVec3(0.5f, 0.5f, 0.0f));

      ezAngle fov;

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
          const ezMat4& viewProjection = pShadowView->GetViewProjectionMatrix(ezCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      float screenHeight = ezMath::Tan(fov * 0.5f) * 20.0f; // screen height in worldspace at 10m distance
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = ezMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      float slopeBias = shadowData.m_fSlopeBias * penumbraSize * ezMath::Tan(fov * 0.5f);
      float constantBias = shadowData.m_fConstantBias * s_uiShadowMapSize / uiShadowMapSize;
      float fadeOut = ezMath::Clamp((shadowData.m_fShadowMapScale - fadeOutEnd) / (fadeOutStart - fadeOutEnd), 0.0f, 1.0f);

      ezUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      ezVec4& shadowParams = packedShadowData[uiParamsIndex];
      shadowParams.x = slopeBias;
      shadowParams.y = constantBias;
      shadowParams.z = penumbraSize * relativeShadowSize;
      shadowParams.w = ezMath::Sqrt(fadeOut);
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (CVarShadowPoolStats)
  {
    ezStringBuilder sb;
    sb.Format("Atlas Utilization: {0}%%", ezArgF(100.0 * (double)uiUsedAtlasSize / uiTotalAtlasSize, 2));

    ezDebugRenderer::Draw2DText(debugContext, sb, ezVec2I32(10, 220), ezColor::LightSteelBlue);
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

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALPass* pGALPass = pDevice->BeginPass("Shadow Atlas");

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(s_pData->m_hShadowAtlasTexture));
  renderingSetup.m_bClearDepth = true;

  auto pCommandEncoder = pGALPass->BeginRendering(renderingSetup);

  ezUInt32 uiDataIndex = ezRenderWorld::GetDataIndexForRendering();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  if (!packedShadowData.IsEmpty())
  {
    EZ_PROFILE_SCOPE("Shadow Data Buffer Update");

    pCommandEncoder->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr());
  }

  pGALPass->EndRendering(pCommandEncoder);
  pDevice->EndPass(pGALPass);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ShadowPool);
