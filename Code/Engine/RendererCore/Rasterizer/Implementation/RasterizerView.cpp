#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
#  include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#  include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>
#endif
#include <RendererCore/Rasterizer/Generic/OccluderGeneric.h>
#include <RendererCore/Rasterizer/Generic/RasterizerGeneric.h>

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
#  include <MaskedOcclusionCulling.h>
#  include <RendererCore/Rasterizer/GenericMOC/GenericMOC.h>
#  include <RendererCore/Rasterizer/MaskedOcclusionCulling/OccluderMOC.h>
#endif

ezCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, ezCVarFlags::Default, "Max resolution for occlusion buffers.");
ezCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, ezCVarFlags::Default, "Max number of occluders to rasterize per frame.");

ezRasterizerView::ezRasterizerView([[maybe_unused]] bool bUseOptimized)
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (bUseOptimized)
    m_Implementation = ezRasterizerImplementation::ThirdParty;
#endif
}

ezRasterizerView::ezRasterizerView(ezRasterizerImplementation impl)
{
  m_Implementation = impl;

#if EZ_DISABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
    m_Implementation = ezRasterizerImplementation::Generic;
#endif

#if !defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling ||
      m_Implementation == ezRasterizerImplementation::GenericMOC)
    m_Implementation = ezRasterizerImplementation::Generic;
#endif
}

ezRasterizerView::~ezRasterizerView()
{
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_pMOC)
  {
    MaskedOcclusionCulling::Destroy(m_pMOC);
    m_pMOC = nullptr;
  }
#endif
}

void ezRasterizerView::SetResolution(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio)
{
  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
    if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling ||
        m_Implementation == ezRasterizerImplementation::GenericMOC)
    {
      if (!m_pMOC)
      {
        if (m_Implementation == ezRasterizerImplementation::GenericMOC)
          m_pMOC = ezCreateGenericMOC();
        else
          m_pMOC = MaskedOcclusionCulling::Create(MaskedOcclusionCulling::Implementation::SSE2);
      }

      m_pMOC->SetResolution(uiWidth, uiHeight);

      m_pRasterizerGeneric.Clear();
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
      m_pRasterizer.Clear();
#endif
    }
    else
#endif
    {
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
      if (m_Implementation == ezRasterizerImplementation::ThirdParty)
      {
        m_pRasterizer = EZ_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);
        m_pRasterizerGeneric.Clear();
      }
      else
#endif
      {
        m_pRasterizerGeneric = EZ_NEW(ezFoundation::GetAlignedAllocator(), RasterizerGeneric, uiWidth, uiHeight);
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
        m_pRasterizer.Clear();
#endif
      }

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
      if (m_pMOC)
      {
        MaskedOcclusionCulling::Destroy(m_pMOC);
        m_pMOC = nullptr;
      }
#endif
    }
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void ezRasterizerView::BeginScene()
{
  EZ_PROFILE_SCOPE("ezRasterizerView::BeginScene");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling || m_Implementation == ezRasterizerImplementation::GenericMOC)
  {
    EZ_ASSERT_DEV(m_pMOC != nullptr, "Call SetResolution() first.");
    m_pMOC->ClearBuffer();
  }
  else
#endif
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
  {
    EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
    m_pRasterizer->clear();
  }
  else
#endif
  {
    EZ_ASSERT_DEV(m_pRasterizerGeneric != nullptr, "Call SetResolution() first.");
    m_pRasterizerGeneric->Clear();
  }

  m_bAnyOccludersRasterized = false;
}

void ezRasterizerView::ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const
{
  EZ_PROFILE_SCOPE("ezRasterizerView::ReadBackFrame");

  EZ_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling || m_Implementation == ezRasterizerImplementation::GenericMOC)
  {
    EZ_ASSERT_DEV(m_pMOC != nullptr, "Call SetResolution() first.");

    const ezUInt32 numPixels = m_uiResolutionX * m_uiResolutionY;
    ezDynamicArray<float> depthData;
    depthData.SetCountUninitialized(numPixels);
    m_pMOC->ComputePixelDepthBuffer(depthData.GetData(), true);

    for (ezUInt32 i = 0; i < numPixels; ++i)
    {
      ezUInt8 v = static_cast<ezUInt8>(ezMath::Clamp(depthData[i] * 255.0f, 0.0f, 255.0f));
      targetBuffer[i] = ezColorLinearUB(v, v, v, 255);
    }
  }
  else
#endif
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
  {
    EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
    m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
  }
  else
#endif
  {
    EZ_ASSERT_DEV(m_pRasterizerGeneric != nullptr, "Call SetResolution() first.");
    m_pRasterizerGeneric->ReadBackDepth(targetBuffer.GetPtr());
  }
}

void ezRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("ezRasterizerView::EndScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  // MOC doesn't need the final VP matrix set since it uses per-object matrices
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling || m_Implementation == ezRasterizerImplementation::GenericMOC)
    return;
#endif

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
    m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
  else
#endif
    m_pRasterizerGeneric->SetModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void ezRasterizerView::RasterizeObjects(ezUInt32 uiMaxObjects)
{
  EZ_PROFILE_SCOPE("ezRasterizerView::RasterizeObjects");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling || m_Implementation == ezRasterizerImplementation::GenericMOC)
  {
    for (const Instance& inst : m_Instances)
    {
      const OccluderMOC& occluder = inst.m_pObject->m_OccluderMOC;
      if (occluder.m_uiNumTriangles == 0)
        continue;

      const ezMat4 mModel = inst.m_Transform.GetAsMat4();
      const ezMat4 mMVP = m_mViewProjection * mModel;

      // Use VertexLayout(16, 4, 8): stride 16, y at offset 4, z at offset 8
      // This reads (x, y, z) from (x, y, z, w=1) vertices, with z as the depth component
      MaskedOcclusionCulling::VertexLayout vtxLayout(16, 4, 8);

      m_pMOC->RenderTriangles(
        occluder.m_Vertices.GetData(),
        occluder.m_Indices.GetData(),
        occluder.m_uiNumTriangles,
        mMVP.m_fElementsCM,
        MaskedOcclusionCulling::BACKFACE_CW,
        MaskedOcclusionCulling::CLIP_PLANE_ALL,
        vtxLayout);

      m_bAnyOccludersRasterized = true;

      if (--uiMaxObjects == 0)
        return;
    }

    return;
  }
#endif

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
  {
    for (ezUInt32 i = 0; i < m_Instances.GetCount(); ++i)
    {
      const Instance& inst = m_Instances[i]; 
      ApplyModelViewProjectionMatrix(inst.m_Transform);

      bool bNeedsClipping;
      const Occluder& occluder = inst.m_pObject->m_Occluder;

      if (m_pRasterizer->queryVisibility(occluder.m_boundsMin, occluder.m_boundsMax, bNeedsClipping))
      {
        m_bAnyOccludersRasterized = true;

        if (bNeedsClipping)
        {
          m_pRasterizer->rasterize<true>(occluder);
        }
        else
        {
          m_pRasterizer->rasterize<false>(occluder);
        }

        if (--uiMaxObjects == 0)
          return;
      }
    }
  }
  else
#endif
  {
    for (const Instance& inst : m_Instances)
    {
      ApplyModelViewProjectionMatrix(inst.m_Transform);

      bool bNeedsClipping;
      const auto& occluder = inst.m_pObject->m_OccluderGeneric;

      if (m_pRasterizerGeneric->QueryVisibility(occluder.m_vBoundsMin, occluder.m_vBoundsMax, bNeedsClipping))
      {
        m_bAnyOccludersRasterized = true;

        if (bNeedsClipping)
        {
          m_pRasterizerGeneric->Rasterize<true>(occluder);
        }
        else
        {
          m_pRasterizerGeneric->Rasterize<false>(occluder);
        }

        if (--uiMaxObjects == 0)
          return;
      }
    }
  }
}

void ezRasterizerView::UpdateViewProjectionMatrix()
{
  ezMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, ezCameraEye::Left, ezClipSpaceDepthRange::ZeroToOne);

  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void ezRasterizerView::ApplyModelViewProjectionMatrix(const ezTransform& modelTransform)
{
  const ezMat4 mModel = modelTransform.GetAsMat4();
  const ezMat4 mMVP = m_mViewProjection * mModel;

  // MOC handles MVP per-call in RasterizeObjects, not here
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
    m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
  else
#endif
    m_pRasterizerGeneric->SetModelViewProjection(mMVP.m_fElementsCM);
}

void ezRasterizerView::SortObjectsFrontToBack()
{
  EZ_PROFILE_SCOPE("ezRasterizerView::SortObjectsFrontToBack");

  const ezVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2)
    {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
}

bool ezRasterizerView::IsVisible(const ezSimdBBox& aabb) const
{
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCulling || m_Implementation == ezRasterizerImplementation::GenericMOC)
  {
    // Transform the 8 corners of the AABB to clip space and find NDC extents + min W
    ezVec3 corners[8];
    {
      const ezSimdVec4f mn = aabb.m_Min;
      const ezSimdVec4f mx = aabb.m_Max;
      float minArr[4], maxArr[4];
      mn.Store<4>(minArr);
      mx.Store<4>(maxArr);

      corners[0] = ezVec3(minArr[0], minArr[1], minArr[2]);
      corners[1] = ezVec3(maxArr[0], minArr[1], minArr[2]);
      corners[2] = ezVec3(minArr[0], maxArr[1], minArr[2]);
      corners[3] = ezVec3(maxArr[0], maxArr[1], minArr[2]);
      corners[4] = ezVec3(minArr[0], minArr[1], maxArr[2]);
      corners[5] = ezVec3(maxArr[0], minArr[1], maxArr[2]);
      corners[6] = ezVec3(minArr[0], maxArr[1], maxArr[2]);
      corners[7] = ezVec3(maxArr[0], maxArr[1], maxArr[2]);
    }

    float ndcMinX = ezMath::MaxValue<float>();
    float ndcMinY = ezMath::MaxValue<float>();
    float ndcMaxX = -ezMath::MaxValue<float>();
    float ndcMaxY = -ezMath::MaxValue<float>();
    float wMin = ezMath::MaxValue<float>();

    for (int i = 0; i < 8; ++i)
    {
      const ezVec4 cs = m_mViewProjection * corners[i].GetAsPositionVec4();

      if (cs.w <= 0.0f)
      {
        // Corner is behind the camera - conservatively assume visible
        return true;
      }

      const float invW = 1.0f / cs.w;
      const float ndcX = cs.x * invW;
      const float ndcY = cs.y * invW;

      ndcMinX = ezMath::Min(ndcMinX, ndcX);
      ndcMinY = ezMath::Min(ndcMinY, ndcY);
      ndcMaxX = ezMath::Max(ndcMaxX, ndcX);
      ndcMaxY = ezMath::Max(ndcMaxY, ndcY);
      wMin = ezMath::Min(wMin, cs.w);
    }

    auto result = m_pMOC->TestRect(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY, wMin);
    return result != MaskedOcclusionCulling::OCCLUDED;
  }
#endif

  ezSimdVec4f vmin = aabb.m_Min;
  ezSimdVec4f vmax = aabb.m_Max;

  // ezSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
    return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#endif
  return m_pRasterizerGeneric->QueryVisibility(vmin, vmax, needsClipping);
}

ezRasterizerView* ezRasterizerViewPool::GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio, bool bUseOptimized)
{
  ezRasterizerImplementation impl = ezRasterizerImplementation::Generic;
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (bUseOptimized)
    impl = ezRasterizerImplementation::ThirdParty;
#endif
  return GetRasterizerView(uiWidth, uiHeight, fAspectRatio, impl);
}

ezRasterizerView* ezRasterizerViewPool::GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio, ezRasterizerImplementation impl)
{
  EZ_PROFILE_SCOPE("ezRasterizerViewPool::GetRasterizerView");

  EZ_LOCK(m_Mutex);

  const float divX = (float)uiWidth / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)uiHeight / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div = ezMath::Max(divX, divY);

  if (div > 1.0)
  {
    uiWidth = (ezUInt32)(uiWidth / div);
    uiHeight = (ezUInt32)(uiHeight / div);
  }

  uiWidth = ezMath::RoundDown(uiWidth, 8);
  uiHeight = ezMath::RoundDown(uiHeight, 8);

  uiWidth = ezMath::Clamp<ezUInt32>(uiWidth, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  uiHeight = ezMath::Clamp<ezUInt32>(uiHeight, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_pRasterizerView->GetResolutionX() == uiWidth &&
        entry.m_pRasterizerView->GetResolutionY() == uiHeight &&
        entry.m_pRasterizerView->GetImplementation() == impl)
    {
      entry.m_bInUse = true;
      entry.m_pRasterizerView->SetResolution(uiWidth, uiHeight, fAspectRatio);
      return entry.m_pRasterizerView.Borrow();
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_pRasterizerView = EZ_DEFAULT_NEW(ezRasterizerView, impl);
  ne.m_pRasterizerView->SetResolution(uiWidth, uiHeight, fAspectRatio);
  ne.m_bInUse = true;

  return ne.m_pRasterizerView.Borrow();
}

void ezRasterizerViewPool::ReturnRasterizerView(ezRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  EZ_PROFILE_SCOPE("ezRasterizerViewPool::ReturnRasterizerView");

  pView->SetCamera(nullptr);

  EZ_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_pRasterizerView.Borrow() == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerView);
