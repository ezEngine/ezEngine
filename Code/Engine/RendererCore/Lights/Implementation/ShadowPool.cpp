#include <PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ShadowPool)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_ENGINE_STARTUP
{
  ezShadowPool::OnEngineStartup();
}

ON_ENGINE_SHUTDOWN
{
  ezShadowPool::OnEngineShutdown();
}

EZ_END_SUBSYSTEM_DECLARATION

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

namespace
{
  struct ShadowView
  {
    ezViewHandle m_hView;
    ezCamera m_Camera;
  };

  static ezMutex s_ShadowViewsMutex;
  static ezDeque<ShadowView> s_ShadowViews;
  static ezUInt32 s_uiUsedViews;

  struct ShadowData
  {
    ezHybridArray<ezViewHandle, 6> m_Views;
    ezUInt32 m_uiType;
    float m_fShadowMapScale;
    float m_fPenumbraSize;
    ezUInt32 m_uiPackedDataOffset; // in 16 bytes steps
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

  static ezMutex s_ShadowDataMutex;
  static ezDeque<ShadowData> s_ShadowData;
  static ezUInt32 s_uiUsedShadowData;
  static ezHashTable<const ezLightComponent*, ezUInt32> s_LightToShadowDataTable;
  static ezDynamicArray<SortedShadowData> s_SortedShadowData;

  static ezDynamicArray<ezVec4> s_PackedShadowData[2];

  static ezUInt32 s_uiShadowAtlasTextureWidth = 4096; ///\todo make this configurable
  static ezUInt32 s_uiShadowAtlasTextureHeight = 4096;
  static ezUInt32 s_uiShadowMapSize = 1024;
  static ezUInt32 s_uiMinShadowMapSize = 64;
  static float s_fFadeOutScaleStart = (s_uiMinShadowMapSize + 1.0f) / s_uiShadowMapSize;
  static float s_fFadeOutScaleEnd = s_fFadeOutScaleStart * 0.5f;
  static ezGALTextureHandle s_hShadowAtlasTexture;
  static ezGALBufferHandle s_hShadowDataBuffer;
  static bool s_bShadowDataBufferUpdated;

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
      return m_uiChildIndices[0] == 0xFFFF && m_uiChildIndices[1] == 0xFFFF &&
        m_uiChildIndices[2] == 0xFFFF && m_uiChildIndices[3] == 0xFFFF;
    }

    ezRectU32 m_Rect;
    ezUInt16 m_uiChildIndices[4];
    ezUInt32 m_uiDataIndex;
  };

  static ezDynamicArray<AtlasCell> s_AtlasCells;

  enum
  {
    MAX_SHADOW_DATA = 256
  };

  static void CreateShadowAtlasTexture()
  {
    if (s_hShadowAtlasTexture.IsInvalidated())
    {
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight, ezGALResourceFormat::D16);

      s_hShadowAtlasTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  static void CreateShadowDataBuffer()
  {
    if (s_hShadowDataBuffer.IsInvalidated())
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferType = ezGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_hShadowDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  static ezViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();

    ezView* pView = nullptr;
    ezViewHandle hView = ezRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(ezCameraUsageHint::Shadow);

    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetDepthStencilTarget(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(s_hShadowAtlasTexture));
    pView->SetRenderTargetSetup(renderTargetSetup);

    pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }")); //ShadowMapRenderPipeline.ezRenderPipelineAsset

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnBeginFrame before rendering.
    pView->SetViewport(ezRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    ///\todo
    /*const ezTag* tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadows");
    pView->m_IncludeTags.Set(*tagCastShadows);*/

    return hView;
  }

  static ShadowView& GetShadowView(ezView*& out_pView)
  {
    EZ_LOCK(s_ShadowViewsMutex);

    if (s_uiUsedViews == s_ShadowViews.GetCount())
    {
      s_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = s_ShadowViews[s_uiUsedViews];
    if (ezRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
    }

    s_uiUsedViews++;
    return shadowView;
  }

  static bool GetDataForExtraction(const ezLightComponent* pLight, float fShadowMapScale, ezUInt32 uiPackedDataSize, ShadowData*& out_pData)
  {
    EZ_LOCK(s_ShadowDataMutex);

    ezUInt32 uiDataIndex = ezInvalidIndex;
    if (pLight != nullptr && s_LightToShadowDataTable.TryGetValue(pLight, uiDataIndex))
    {
      out_pData = &s_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = ezMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    if (s_uiUsedShadowData == s_ShadowData.GetCount())
    {
      s_ShadowData.SetCount(s_uiUsedShadowData + 1);
    }

    auto& packedShadowData = s_PackedShadowData[ezRenderWorld::GetDataIndexForExtraction()];
    ezUInt32 uiPackedDataOffset = packedShadowData.GetCount();
    packedShadowData.SetCountUninitialized(uiPackedDataOffset + uiPackedDataSize / sizeof(ezVec4));

    out_pData = &s_ShadowData[s_uiUsedShadowData];
    out_pData->m_fShadowMapScale = fShadowMapScale;
    out_pData->m_fPenumbraSize = pLight->GetPenumbraSize();
    out_pData->m_uiPackedDataOffset = uiPackedDataOffset;

    if (pLight != nullptr)
    {
      s_LightToShadowDataTable.Insert(pLight, s_uiUsedShadowData);
    }

    ++s_uiUsedShadowData;

    return false;
  }

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
      s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x,     y,     w, h);
      s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x + w, y,     w, h);
      s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(x,     y + h, w, h);
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
}

//static
ezUInt32 ezShadowPool::AddDirectionalLight(const ezDirectionalLightComponent* pDirLight)
{
  EZ_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  return ezInvalidIndex;
}

//static
ezUInt32 ezShadowPool::AddPointLight(const ezPointLightComponent* pPointLight, float fScreenSpaceSize)
{
  EZ_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd * 2.0f)
  {
    return ezInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (GetDataForExtraction(pPointLight, fScreenSpaceSize, sizeof(ezPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  ezVec3 faceDirs[6] =
  {
    ezVec3(1.0f, 0.0f, 0.0f),
    ezVec3(-1.0f, 0.0f, 0.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, -1.0f),
  };

  const char* viewNames[6] =
  {
    "PointLightView+X",
    "PointLightView-X",
    "PointLightView+Y",
    "PointLightView-Y",
    "PointLightView+Z",
    "PointLightView-Z",
  };

  float fPenumbraSize = ezMath::Max(pPointLight->GetPenumbraSize(), (0.5f / s_uiMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(ezAngle::Degree(90.0f), fPenumbraSize);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezView* pView = nullptr;
    ShadowView& shadowView = GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<ezWorld*>(pPointLight->GetWorld()));
    }

    // Setup camera
    {
      const ezGameObject* pOwner = pPointLight->GetOwner();
      ezVec3 vPosition = pOwner->GetGlobalPosition();
      ezVec3 vForward = faceDirs[i];
      ezVec3 vUp = ezVec3(0.0f, 0.0f, 1.0f);

      float fNearPlane = 0.1f; ///\todo expose somewhere
      float fFarPlane = pPointLight->GetEffectiveRange();

      ezCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    ezRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

//static
ezUInt32 ezShadowPool::AddSpotLight(const ezSpotLightComponent* pSpotLight, float fScreenSpaceSize)
{
  EZ_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd)
  {
    return ezInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (GetDataForExtraction(pSpotLight, fScreenSpaceSize, sizeof(ezSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  ezView* pView = nullptr;
  ShadowView& shadowView = GetShadowView(pView);
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
    ezVec3 vForward = pOwner->GetDirForwards();
    ezVec3 vUp = pOwner->GetDirUp();

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

//static
ezGALTextureHandle ezShadowPool::GetShadowAtlasTexture()
{
  return s_hShadowAtlasTexture;
}

//static
float ezShadowPool::GetShadowAtlasTexelSize()
{
  return 1.0f / s_uiShadowAtlasTextureWidth;
}

//static
ezGALBufferHandle ezShadowPool::UpdateShadowDataBuffer(ezGALContext* pGALContext)
{
  if (!s_bShadowDataBufferUpdated)
  {
    auto& packedShadowData = s_PackedShadowData[ezRenderWorld::GetDataIndexForRendering()];
    if (!packedShadowData.IsEmpty())
    {
      CreateShadowDataBuffer();

      pGALContext->UpdateBuffer(s_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr());
    }

    s_bShadowDataBufferUpdated = true;
  }

  return s_hShadowDataBuffer;
}

//static
void ezShadowPool::OnEngineStartup()
{
  s_PackedShadowData[0] = ezDynamicArray<ezVec4, ezAlignedAllocatorWrapper>();
  s_PackedShadowData[1] = ezDynamicArray<ezVec4, ezAlignedAllocatorWrapper>();

  ezRenderWorld::s_BeginFrameEvent.AddEventHandler(OnBeginFrame);
}

//static
void ezShadowPool::OnEngineShutdown()
{
  ezRenderWorld::s_BeginFrameEvent.RemoveEventHandler(OnBeginFrame);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  if (!s_hShadowAtlasTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(s_hShadowAtlasTexture);
    s_hShadowAtlasTexture.Invalidate();
  }

  if (!s_hShadowDataBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(s_hShadowDataBuffer);
    s_hShadowDataBuffer.Invalidate();
  }
}

//static
void ezShadowPool::OnBeginFrame(ezUInt64 uiFrameNumber)
{
  EZ_PROFILE("Shadow Pool Update");

  if (s_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (ezUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_ShadowData[uiShadowDataIndex];

    auto& sorted = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_uiType == LIGHT_TYPE_DIR ? 100.0f : ezMath::Min(shadowData.m_fShadowMapScale, 10.0f);
  }

  s_SortedShadowData.Sort();

  // Fill atlas and collect shadow data
  s_AtlasCells.Clear();
  s_AtlasCells.ExpandAndGetRef().m_Rect = ezRectU32(0, 0, s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight);

  float fAtlasInvWidth = 1.0f / s_uiShadowAtlasTextureWidth;
  float fAtlasInvHeight = 1.0f / s_uiShadowAtlasTextureWidth;

  ezMat4 texScaleMatrix;
  texScaleMatrix.SetIdentity();
  texScaleMatrix.SetDiagonal(ezVec4(0.5f, -0.5f, 1.0f, 1.0f));
  texScaleMatrix.SetTranslationVector(ezVec3(0.5f, 0.5f, 0.0f));

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezUInt32 uiTotalAtlasSize = s_uiShadowAtlasTextureWidth * s_uiShadowAtlasTextureHeight;
  ezUInt32 uiUsedAtlasSize = 0;

  ezDebugRendererContext debugContext(ezWorld::GetWorld(0));

  if (CVarShadowPoolStats)
  {
    ezDebugRenderer::DrawText(debugContext, "Shadow Pool Stats", ezVec2I32(10, 200), ezColor::LightSteelBlue);
    ezDebugRenderer::DrawText(debugContext, "Details (Name: Size - Atlas Offset)", ezVec2I32(10, 250), ezColor::LightSteelBlue);
  }

  ezInt32 iCurrentStatsOffset = 270;
#endif

  auto& packedShadowData = s_PackedShadowData[ezRenderWorld::GetDataIndexForRendering()];

  for (auto& sorted : s_SortedShadowData)
  {
    ezUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_ShadowData[uiShadowDataIndex];

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
    ezAngle fov;

    for (ezUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      ezView* pShadowView = nullptr;
      if (!ezRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView))
      {
        continue;
      }

      ezRectU32 atlasRect = FindAtlasRect(uiShadowMapSize, uiShadowDataIndex);
      pShadowView->SetViewport(ezRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));

      #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (CVarShadowPoolStats)
        {
          ezStringBuilder sb;
          sb.Format("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y);

          ezDebugRenderer::DrawText(debugContext, sb, ezVec2I32(10, iCurrentStatsOffset), ezColor::LightSteelBlue);
          iCurrentStatsOffset += 20;

          uiUsedAtlasSize += atlasRect.width * atlasRect.height;
        }
      #endif

      ezVec2 scale = ezVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
      ezVec2 offset = ezVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

      ezMat4 atlasMatrix;
      atlasMatrix.SetIdentity();
      atlasMatrix.SetDiagonal(ezVec4(scale.x, scale.y, 1.0f, 1.0f));
      atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

      fov = pShadowView->GetCamera()->GetFovY(1.0f);
      const ezMat4& viewProjection = pShadowView->GetViewProjectionMatrix();

      ezUInt32 uiMatrixOffset = GET_WORLD_TO_LIGHT_MATRIX_OFFSET(shadowData.m_uiPackedDataOffset, uiViewIndex);
      ezMat4& worldToLightMatrix = *reinterpret_cast<ezMat4*>(&packedShadowData[uiMatrixOffset]);
      worldToLightMatrix = atlasMatrix * texScaleMatrix * viewProjection;
    }

    float screenHeight = ezMath::Tan(fov * 0.5f) * 2.0f;
    float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

    float penumbraSize = ezMath::Max((shadowData.m_fPenumbraSize / screenHeight) * relativeShadowSize, fAtlasInvHeight);
    float fadeOut = ezMath::Clamp((shadowData.m_fShadowMapScale - fadeOutEnd) / (fadeOutStart - fadeOutEnd), 0.0f, 1.0f);

    ezUInt32 uiParamsOffset = GET_SHADOW_PARAMS_OFFSET(shadowData.m_uiPackedDataOffset);
    ezVec4& shadowParams = packedShadowData[uiParamsOffset];
    shadowParams.x = 0.001f;
    shadowParams.y = 0.0004f * (fov / ezAngle::Degree(90.0f).GetRadian()).GetRadian();
    shadowParams.z = penumbraSize;
    shadowParams.w = ezMath::Sqrt(fadeOut);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (CVarShadowPoolStats)
  {
    ezStringBuilder sb;
    sb.Format("Atlas Utilization: {0}%%", ezArgF(100.0 * (double)uiUsedAtlasSize / uiTotalAtlasSize, 2));

    ezDebugRenderer::DrawText(debugContext, sb, ezVec2I32(10, 220), ezColor::LightSteelBlue);
  }
#endif

  s_bShadowDataBufferUpdated = false;

  s_uiUsedViews = 0;
  s_uiUsedShadowData = 0;
  s_LightToShadowDataTable.Clear();

  s_PackedShadowData[ezRenderWorld::GetDataIndexForExtraction()].Clear();

  // clear atlas texture
  if (!s_hShadowAtlasTexture.IsInvalidated())
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALContext* pGALContext = pDevice->GetPrimaryContext();

    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(s_hShadowAtlasTexture));

    pGALContext->SetRenderTargetSetup(renderTargetSetup);
    pGALContext->Clear(ezColor::White);
  }
}

