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
#endif
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
#  include <MaskedOcclusionCulling.h>
#  include <RendererCore/Rasterizer/GenericMOC/GenericMOC.h>
#  include <RendererCore/Rasterizer/MaskedOcclusionCulling/OccluderMOC.h>
#endif

ezCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, ezCVarFlags::Default, "Max resolution for occlusion buffers.");
ezCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, ezCVarFlags::Default, "Max number of occluders to rasterize per frame.");

ezRasterizerView::ezRasterizerView(bool bUseOptimized)
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (bUseOptimized)
    m_Implementation = ezRasterizerImplementation::ThirdParty;
#  if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  else
    m_Implementation = ezRasterizerImplementation::GenericMOC;
#  endif
#else
  EZ_IGNORE_UNUSED(bUseOptimized);
#  if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  m_Implementation = ezRasterizerImplementation::GenericMOC;
#  endif
#endif
}

ezRasterizerView::ezRasterizerView(ezRasterizerImplementation impl)
{
  m_Implementation = impl;

#if EZ_DISABLED(EZ_RASTERIZER_SUPPORTED) && defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation != ezRasterizerImplementation::GenericMOC)
    m_Implementation = ezRasterizerImplementation::GenericMOC;
#endif

#if !defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_Implementation == ezRasterizerImplementation::GenericMOC ||
      m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingSSE2 ||
      m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingSSE41 ||
      m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingAVX2 ||
      m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingAVX512)
    m_Implementation = ezRasterizerImplementation::ThirdParty;
#endif
}

ezRasterizerView::~ezRasterizerView()
{
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_pMOC)
  {
    if (m_Implementation == ezRasterizerImplementation::GenericMOC)
      ezDestroyGenericMOC(m_pMOC);
#  if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
    else
      MaskedOcclusionCulling::Destroy(m_pMOC);
#  endif
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
    if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
    {
      if (!m_pMOC)
      {
        if (m_Implementation == ezRasterizerImplementation::GenericMOC)
          m_pMOC = ezCreateGenericMOC();
        else
        {
#  if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
          // The indices match the values of MaskedOcclusionCulling::Implementation.
          m_pMOC = MaskedOcclusionCulling::Create(static_cast<MaskedOcclusionCulling::Implementation>(ezMOCInstructionSetIndex(m_Implementation)));
#  endif
        }
      }

      m_pMOC->SetResolution(uiWidth, uiHeight);

      m_pRasterizer.Clear();
    }
    else
#endif
    {
      m_pRasterizer = EZ_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
      if (m_pMOC)
      {
#  if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
        MaskedOcclusionCulling::Destroy(m_pMOC);
#  endif
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

ezRasterizerImplementation ezRasterizerView::GetMOCInstructionSet() const
{
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  // The portable port reports SSE2, but it is a distinct implementation and shouldn't be mistaken for the native one.
  if (m_pMOC && m_Implementation != ezRasterizerImplementation::GenericMOC)
  {
    const ezInt32 mocImpl = m_pMOC->GetImplementation();
    for (ezInt32 i = 0; i < (ezInt32)sizeof(ezMOCInstructionSets) / sizeof(ezMOCInstructionSets[0]); ++i)
    {
      if (i == mocImpl)
        return ezMOCInstructionSets[i];
    }
  }
#endif

  return m_Implementation;
}

void ezRasterizerView::BeginScene()
{
  EZ_PROFILE_SCOPE("ezRasterizerView::BeginScene");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    EZ_ASSERT_DEV(m_pMOC != nullptr, "Call SetResolution() first.");
    m_pMOC->ClearBuffer();
  }
  else
#endif
  {
    EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
    m_pRasterizer->clear();
  }

  m_bAnyOccludersRasterized = false;
}

void ezRasterizerView::ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const
{
  EZ_PROFILE_SCOPE("ezRasterizerView::ReadBackFrame");

  EZ_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
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
  {
    EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
    m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
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
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
    return;
#endif

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void ezRasterizerView::RasterizeObjects(ezUInt32 uiMaxObjects)
{
  EZ_PROFILE_SCOPE("ezRasterizerView::RasterizeObjects");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    for (const Instance& inst : m_Instances)
    {
      const OccluderMOC& occluder = inst.m_pObject->m_OccluderMOC;
      if (occluder.m_uiNumTriangles == 0)
        continue;

      const ezMat4 mModel = inst.m_Transform.GetAsMat4();
      alignas(16)
        const ezMat4 mMVP = m_mViewProjection * mModel;

      if (m_bAnyOccludersRasterized && !IsVisibleMOC(ezBoundingBox::MakeFromMinMax(occluder.m_vBoundsMin, occluder.m_vBoundsMax), mMVP))
        continue;

      // Use VertexLayout(16, 4, 8): stride 16, y at offset 4, z at offset 8
      // This reads (x, y, z) from (x, y, z, w=1) vertices, with z as the depth component
      MaskedOcclusionCulling::VertexLayout vtxLayout(16, 4, 8);

      m_pMOC->RenderTriangles(
        occluder.m_Vertices.GetData(),
        occluder.m_Indices.GetData(),
        occluder.m_uiNumTriangles,
        mMVP.m_fElementsCM,
        MaskedOcclusionCulling::BACKFACE_CCW,
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
#endif
}

void ezRasterizerView::UpdateViewProjectionMatrix()
{
  ezMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, ezCameraEye::Left, ezClipSpaceDepthRange::ZeroToOne);

  // The software rasterizer expects near depth 0 and far depth 1.
  // depth = clipZ / clipW. Row 2 produces clipZ; row 3 produces clipW. Replacing row 2 with row3 - row2 therefore flips depth.
  mProjection.SetRow(2, mProjection.GetRow(3) - mProjection.GetRow(2));
  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void ezRasterizerView::ApplyModelViewProjectionMatrix(const ezTransform& modelTransform)
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  const ezMat4 mModel = modelTransform.GetAsMat4();
  const ezMat4 mMVP = m_mViewProjection * mModel;

  if (m_Implementation == ezRasterizerImplementation::ThirdParty)
    m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
#else
  EZ_IGNORE_UNUSED(modelTransform);
#endif
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
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    return IsVisibleMOC(ezSimdConversion::ToBBox(aabb), m_mViewProjection);
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
#else
  EZ_IGNORE_UNUSED(vmin);
  EZ_IGNORE_UNUSED(vmax);
  EZ_IGNORE_UNUSED(needsClipping);
  return true;
#endif
  return true;
}

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
bool ezRasterizerView::IsVisibleMOC(const ezBoundingBox& bounds, const ezMat4& mModelViewProjection) const
{
  const ezVec3& vMin = bounds.m_vMin;
  const ezVec3& vMax = bounds.m_vMax;
  // Each lane projects one corner; the two Z planes cover all eight corners.
  const ezSimdVec4f vX(vMin.x, vMax.x, vMin.x, vMax.x);
  const ezSimdVec4f vY(vMin.y, vMin.y, vMax.y, vMax.y);
  const ezVec4 vRowX = mModelViewProjection.GetRow(0);
  const ezVec4 vRowY = mModelViewProjection.GetRow(1);
  const ezVec4 vRowZ = mModelViewProjection.GetRow(2);
  const ezVec4 vRowW = mModelViewProjection.GetRow(3);
  const ezSimdVec4f vBaseX = vX * ezSimdFloat(vRowX.x) + vY * ezSimdFloat(vRowX.y);
  const ezSimdVec4f vBaseY = vX * ezSimdFloat(vRowY.x) + vY * ezSimdFloat(vRowY.y);
  const ezSimdVec4f vBaseZ = vX * ezSimdFloat(vRowZ.x) + vY * ezSimdFloat(vRowZ.y);
  const ezSimdVec4f vBaseW = vX * ezSimdFloat(vRowW.x) + vY * ezSimdFloat(vRowW.y);

  ezSimdVec4f vMinX(ezMath::MaxValue<float>());
  ezSimdVec4f vMinY(ezMath::MaxValue<float>());
  ezSimdVec4f vMaxX(-ezMath::MaxValue<float>());
  ezSimdVec4f vMaxY(-ezMath::MaxValue<float>());
  ezSimdVec4f vMinW(ezMath::MaxValue<float>());
  const float fZPlanes[] = {vMin.z, vMax.z};
  for (float fZ : fZPlanes)
  {
    const ezSimdVec4f vClipW = (vBaseW + ezSimdVec4f(fZ * vRowW.z)) + ezSimdVec4f(vRowW.w);
    const ezSimdVec4f vClipZ = (vBaseZ + ezSimdVec4f(fZ * vRowZ.z)) + ezSimdVec4f(vRowZ.w);
    if (((vClipW <= ezSimdVec4f::MakeZero()) || (vClipZ < ezSimdVec4f::MakeZero())).AnySet<4>())
      return true; // Conservatively keep bounds crossing the camera or near plane.

    const ezSimdVec4f vInvW = vClipW.GetReciprocal();
    const ezSimdVec4f vNdcX = ((vBaseX + ezSimdVec4f(fZ * vRowX.z)) + ezSimdVec4f(vRowX.w)).CompMul(vInvW);
    const ezSimdVec4f vNdcY = ((vBaseY + ezSimdVec4f(fZ * vRowY.z)) + ezSimdVec4f(vRowY.w)).CompMul(vInvW);
    vMinX = vMinX.CompMin(vNdcX);
    vMinY = vMinY.CompMin(vNdcY);
    vMaxX = vMaxX.CompMax(vNdcX);
    vMaxY = vMaxY.CompMax(vNdcY);
    vMinW = vMinW.CompMin(vClipW);
  }

  return m_pMOC->TestRect(vMinX.HorizontalMin<4>(), vMinY.HorizontalMin<4>(), vMaxX.HorizontalMax<4>(), vMaxY.HorizontalMax<4>(), vMinW.HorizontalMin<4>()) != MaskedOcclusionCulling::OCCLUDED;
}
#endif

ezRasterizerView* ezRasterizerViewPool::GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio, bool bUseOptimized)
{
  ezRasterizerImplementation impl = ezRasterizerImplementation::ThirdParty;
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (!bUseOptimized)
  {
#  if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
    impl = ezRasterizerImplementation::GenericMOC;
#  endif
  }
#else
  EZ_IGNORE_UNUSED(bUseOptimized);
#  if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  impl = ezRasterizerImplementation::GenericMOC;
#  endif
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
