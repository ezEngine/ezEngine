#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>

#include <Foundation/System/SystemInformation.h>

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
#  include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#endif
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
#  include <MaskedOcclusionCulling.h>
#  include <RendererCore/Rasterizer/MaskedOcclusionCulling/GenericMOC.h>
#  include <RendererCore/Rasterizer/MaskedOcclusionCulling/OccluderMOC.h>
#endif

ezCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, ezCVarFlags::Default, "Max resolution for occlusion buffers.");
ezCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, ezCVarFlags::Default, "Max number of occluders to rasterize per frame.");
ezCVarInt cvar_SpatialCullingOcclusionImplementation("Spatial.Occlusion.Implementation", 5, ezCVarFlags::Default, "Actual ezRasterizerImplementation to use for ezRasterizerView set to `Auto`");
ezCVarFloat cvar_SpatialCullingOcclusionReadbackFarDistance("Spatial.Occlusion.ReadbackFarDistance", 100.0f, ezCVarFlags::Default, "View distance in meters at which the occlusion buffer debug view reaches black.");

bool ezIsRasterizerImplementationSupported(ezRasterizerImplementation impl)
{
  switch (impl)
  {
    case ezRasterizerImplementation::ThirdParty:
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
    {
      const ezCpuFeatures& cpuFeatures = ezSystemInformation::Get().GetCpuFeatures();
      return cpuFeatures.IsAvx2Available() && cpuFeatures.HW_FMA3;
    }
#else
      return false;
#endif

    case ezRasterizerImplementation::MaskedOcclusionCullingSSE2:
    case ezRasterizerImplementation::MaskedOcclusionCullingSSE41:
    case ezRasterizerImplementation::MaskedOcclusionCullingAVX2:
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT) && EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
    {
      const ezCpuFeatures& cpuFeatures = ezSystemInformation::Get().GetCpuFeatures();
      if (impl == ezRasterizerImplementation::MaskedOcclusionCullingSSE2)
        return cpuFeatures.HW_SSE2;
      if (impl == ezRasterizerImplementation::MaskedOcclusionCullingSSE41)
        return cpuFeatures.HW_SSE41;
      return cpuFeatures.IsAvx2Available();
    }
#else
      return false;
#endif

    case ezRasterizerImplementation::MaskedOcclusionCullingEZ:
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
      return true;
#else
      return false;
#endif

    case ezRasterizerImplementation::Auto:
      return false;
  }

  return false;
}

namespace
{
  /// Adjusts a requested implementation to one that is actually available in this build.
  ezRasterizerImplementation NormalizeRasterizerImplementation(ezRasterizerImplementation impl)
  {
#if EZ_DISABLED(EZ_RASTERIZER_SUPPORTED) && defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
    if (impl != ezRasterizerImplementation::MaskedOcclusionCullingEZ)
      impl = ezRasterizerImplementation::MaskedOcclusionCullingEZ;
#endif

#if !defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
    if (ezIsMaskedOcclusionCullingImplementation(impl))
      impl = ezRasterizerImplementation::ThirdParty;
#endif

    // The native MaskedOcclusionCulling implementations are only compiled for x86.
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT) && EZ_DISABLED(EZ_PLATFORM_ARCH_X86)
    if (ezMOCInstructionSetIndex(impl) >= 0)
      impl = ezRasterizerImplementation::MaskedOcclusionCullingEZ;
#endif

    return impl;
  }

  /// Turns the `Spatial.Occlusion.Implementation` CVar into a concrete rasterizer implementation.
  ///
  /// Values 0 to 4 map directly to ezRasterizerImplementation. Any other value, in particular Auto,
  /// selects the fastest MaskedOcclusionCulling implementation that this CPU supports.
  ezRasterizerImplementation ResolveAutoRasterizerImplementation()
  {
    const ezInt32 cvarValue = cvar_SpatialCullingOcclusionImplementation.GetValue();

    if (cvarValue >= 0 && cvarValue <= (ezInt32)ezRasterizerImplementation::MaskedOcclusionCullingEZ)
      return static_cast<ezRasterizerImplementation>(cvarValue);

    for (ezInt32 i = EZ_ARRAY_SIZE(ezMOCInstructionSets) - 1; i >= 0; --i)
    {
      if (ezIsRasterizerImplementationSupported(ezMOCInstructionSets[i]))
        return ezMOCInstructionSets[i];
    }

    return ezRasterizerImplementation::MaskedOcclusionCullingEZ;
  }
} // namespace

ezRasterizerView::ezRasterizerView(ezRasterizerImplementation impl)
{
  const bool bResolveFromCVar = impl == ezRasterizerImplementation::Auto;
  if (bResolveFromCVar)
    impl = ResolveAutoRasterizerImplementation();

  m_Implementation = NormalizeRasterizerImplementation(impl);

  if (bResolveFromCVar)
  {
    m_uiImplementationSubscription = cvar_SpatialCullingOcclusionImplementation.m_CVarEvents.AddEventHandler(
        ezDelegate<void(const ezCVarEvent&)>(&ezRasterizerView::OnOcclusionImplementationChanged, this));
  }
}

ezRasterizerView::~ezRasterizerView()
{
  cvar_SpatialCullingOcclusionImplementation.m_CVarEvents.RemoveEventHandler(m_uiImplementationSubscription);

  DestroyRasterizer();
}

void ezRasterizerView::OnOcclusionImplementationChanged(const ezCVarEvent& event)
{
  if (event.m_EventType != ezCVarEvent::ValueChanged)
    return;

  // Applied immediately, so the view must not be in the middle of a scene when the CVar changes.
  // The pool hands out a view only between BeginScene() and EndScene(), and CVars are changed on the main thread.
  ApplyImplementation(ResolveAutoRasterizerImplementation());
}

void ezRasterizerView::ApplyImplementation(ezRasterizerImplementation impl)
{
  impl = NormalizeRasterizerImplementation(impl);
  if (impl == m_Implementation)
    return;

  // m_Implementation identifies how the existing instance has to be destroyed,
  // therefore it may only be changed after that instance is gone.
  DestroyRasterizer();
  m_Implementation = impl;

  if (m_uiResolutionX > 0 && m_uiResolutionY > 0)
    CreateRasterizer();

  // The new instance has an empty depth buffer, so nothing is occluded yet.
  m_bAnyOccludersRasterized = false;
}

void ezRasterizerView::CreateRasterizer()
{
#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    if (!m_pMOC)
    {
      if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingEZ)
        m_pMOC = ezUniquePtr<MaskedOcclusionCulling>(ezCreateGenericMOC(), nullptr);
#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
      else
      {
        // The indices match the values of MaskedOcclusionCulling::Implementation.
        // A requested instruction set that the CPU doesn't support is downgraded by the library.
        m_pMOC = ezUniquePtr<MaskedOcclusionCulling>(
          MaskedOcclusionCulling::Create(
            static_cast<MaskedOcclusionCulling::Implementation>(ezMOCInstructionSetIndex(m_Implementation))),
          nullptr);
      }
#endif
    }

    EZ_ASSERT_DEV(m_pMOC != nullptr, "No rasterizer instance available for implementation %d.", (ezInt32)m_Implementation);

    m_pMOC->SetResolution(m_uiResolutionX, m_uiResolutionY);
    m_pMOC->SetNearClipPlane(m_fNearClipW);

    m_pRasterizer.Clear();
  }
  else
#endif
  {
    m_pRasterizer = EZ_DEFAULT_NEW(Rasterizer, m_uiResolutionX, m_uiResolutionY);
  }
}

void ezRasterizerView::DestroyRasterizer()
{
  m_pRasterizer.Clear();

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (m_pMOC)
  {
    MaskedOcclusionCulling* pMOC = m_pMOC.Release();
    if (m_Implementation == ezRasterizerImplementation::MaskedOcclusionCullingEZ)
      ezDestroyGenericMOC(pMOC);
#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
    else
      MaskedOcclusionCulling::Destroy(pMOC);
#endif
  }
#endif
}

void ezRasterizerView::SetResolution(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio)
{
  EZ_ASSERT_DEV(uiWidth % 4 == 0 && uiHeight % 4 == 0, "Rasterizer resolution must be a multiple of 4 in both dimensions.");

  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
    if (!ezIsMaskedOcclusionCullingImplementation(m_Implementation) && m_pMOC)
      DestroyRasterizer();
#endif

    CreateRasterizer();
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
  if (m_pMOC && m_Implementation != ezRasterizerImplementation::MaskedOcclusionCullingEZ)
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

namespace
{
  EZ_ALWAYS_INLINE ezColorLinearUB DistanceToGray(float fDistance, float fNearDistance, float fFarDistance)
  {
    const float fRange = ezMath::Max(fFarDistance - fNearDistance, 0.001f);
    const float fBrightness = 1.0f - ezMath::Saturate((fDistance - fNearDistance) / fRange);
    const ezUInt8 uiGray = static_cast<ezUInt8>(fBrightness * 255.0f + 0.5f);
    return ezColorLinearUB(uiGray, uiGray, uiGray, 255);
  }
} // namespace

void ezRasterizerView::ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const
{
  EZ_PROFILE_SCOPE("ezRasterizerView::ReadBackFrame");

  EZ_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  const float fFarDistance = cvar_SpatialCullingOcclusionReadbackFarDistance;
  const ezUInt32 uiNumPixels = m_uiResolutionX * m_uiResolutionY;
  ezDynamicArray<float> depthData;
  depthData.SetCountUninitialized(uiNumPixels);

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    EZ_ASSERT_DEV(m_pMOC != nullptr, "Call SetResolution() first.");

    // Flip the MOC buffer to match the ThirdParty readback orientation. Negative values are uncovered pixels.
    m_pMOC->ComputePixelDepthBuffer(depthData.GetData(), true);

    for (ezUInt32 i = 0; i < uiNumPixels; ++i)
    {
      const float fInverseW = depthData[i];

      if (fInverseW < 0.0f)
        targetBuffer[i] = ezColorLinearUB(0, 0, 0, 0);
      else if (!m_bReadbackPerspective || fInverseW == 0.0f)
        targetBuffer[i] = ezColorLinearUB(255, 255, 255, 255); // Orthographic clip W is always one, so there is no depth information.
      else
        targetBuffer[i] = DistanceToGray(1.0f / fInverseW, m_fReadbackNearDistance, fFarDistance);
    }
  }
  else
#endif
  {
    EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
    m_pRasterizer->readBackDepth(depthData.GetData());

    for (ezUInt32 i = 0; i < uiNumPixels; ++i)
    {
      // The ThirdParty depth is half the reversed projected depth of the unmodified engine projection.
      const float fDepth2 = 2.0f * depthData[i];

      if (depthData[i] <= 0.0f)
      {
        targetBuffer[i] = ezColorLinearUB(0, 0, 0, 0);
        continue;
      }

      float fDistance;
      if (m_bReadbackPerspective)
        fDistance = m_fReadbackDepthRowW / (fDepth2 - m_fReadbackDepthRowZ);
      else
        fDistance = ezMath::Abs((fDepth2 - m_fReadbackDepthRowW) / m_fReadbackDepthRowZ);

      targetBuffer[i] = DistanceToGray(fDistance, m_fReadbackNearDistance, fFarDistance);
    }
  }
}

ezResult ezRasterizerView::ReadBackDepth(ezArrayPtr<float> targetBuffer) const
{
  EZ_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    EZ_ASSERT_DEV(m_pMOC != nullptr, "Call SetResolution() first.");
    // MOC stores the top row first, 'flipY' would turn the image upside down.
    m_pMOC->ComputePixelDepthBuffer(targetBuffer.GetPtr(), false);
    return EZ_SUCCESS;
  }
#endif

  EZ_IGNORE_UNUSED(targetBuffer);
  return EZ_FAILURE;
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

  // ReadBackFrame converts the stored depth back to a view distance with the unmodified projection.
  const ezVec4 vDepthRow = mProjection.GetRow(2);
  m_bReadbackPerspective = m_pCamera->IsPerspective();
  m_fReadbackDepthRowZ = vDepthRow.z;
  m_fReadbackDepthRowW = vDepthRow.w;
  m_fReadbackNearDistance = m_pCamera->GetNearPlane();

#if defined(BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  if (ezIsMaskedOcclusionCullingImplementation(m_Implementation))
  {
    // MOC ignores clip z and uses 1 / clipW as reversed depth, so the engine projection is used as is.
    // Clip w is the view distance for perspective projections, so the camera near plane can be passed directly.
    // Orthographic projections have a constant w of 1 and keep the clip plane at w = 0.
    m_fNearClipW = m_pCamera->IsPerspective() ? m_pCamera->GetNearPlane() : 0.0f;
    m_pMOC->SetNearClipPlane(m_fNearClipW);

  }
  else
#endif
  {
    // The software rasterizer expects near depth 0 and far depth 1.
    // depth = clipZ / clipW. Row 2 produces clipZ; row 3 produces clipW. Replacing row 2 with row3 - row2 therefore flips depth.
    mProjection.SetRow(2, mProjection.GetRow(3) - mProjection.GetRow(2));
  }

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

  const ezVec3 vCamPos = m_pCamera->GetCenterPosition();

  for (Instance& inst : m_Instances)
  {
    const ezTransform& t = inst.m_Transform;
    inst.m_fSortOriginDistanceSq = (t.m_vPosition - vCamPos).GetLengthSquared();
    inst.m_fSortDistanceSq = inst.m_fSortOriginDistanceSq;

    const ezBoundingBox& localBounds = inst.m_pObject->m_LocalBounds;
    if (!localBounds.IsValid() || t.m_vScale.x == 0.0f || t.m_vScale.y == 0.0f || t.m_vScale.z == 0.0f)
      continue;

    // Closest point on the oriented box: clamp the camera position in object space and transform it back.
    // This is exact for any rotation and non-uniform scale, and zero when the camera is inside the box.
    const ezVec3 vLocalCam = (t.m_qRotation.GetInverse() * (vCamPos - t.m_vPosition)).CompDiv(t.m_vScale);
    const ezVec3 vClosest = t.TransformPosition(localBounds.GetClampedPoint(vLocalCam));
    inst.m_fSortDistanceSq = (vClosest - vCamPos).GetLengthSquared();
  }

  m_Instances.Sort([](const Instance& i1, const Instance& i2)
    {
      if (i1.m_fSortDistanceSq != i2.m_fSortDistanceSq)
        return i1.m_fSortDistanceSq < i2.m_fSortDistanceSq;

      // Objects that contain the camera all have distance zero, prefer the ones whose center is closer.
      return i1.m_fSortOriginDistanceSq < i2.m_fSortOriginDistanceSq; });
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
  // MOC only uses clip x, y and w; depth is 1 / w.
  const ezVec4 vRowX = mModelViewProjection.GetRow(0);
  const ezVec4 vRowY = mModelViewProjection.GetRow(1);
  const ezVec4 vRowW = mModelViewProjection.GetRow(3);
  const ezSimdVec4f vBaseX = vX * ezSimdFloat(vRowX.x) + vY * ezSimdFloat(vRowX.y);
  const ezSimdVec4f vBaseY = vX * ezSimdFloat(vRowY.x) + vY * ezSimdFloat(vRowY.y);
  const ezSimdVec4f vBaseW = vX * ezSimdFloat(vRowW.x) + vY * ezSimdFloat(vRowW.y);
  const ezSimdVec4f vNearClipW(m_fNearClipW);

  ezSimdVec4f vMinX(ezMath::MaxValue<float>());
  ezSimdVec4f vMinY(ezMath::MaxValue<float>());
  ezSimdVec4f vMaxX(-ezMath::MaxValue<float>());
  ezSimdVec4f vMaxY(-ezMath::MaxValue<float>());
  ezSimdVec4f vMinW(ezMath::MaxValue<float>());
  const float fZPlanes[] = {vMin.z, vMax.z};
  for (float fZ : fZPlanes)
  {
    const ezSimdVec4f vClipW = (vBaseW + ezSimdVec4f(fZ * vRowW.z)) + ezSimdVec4f(vRowW.w);
    if ((vClipW <= vNearClipW).AnySet<4>())
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

ezRasterizerView* ezRasterizerViewPool::GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio)
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
        entry.m_pRasterizerView->GetResolutionY() == uiHeight)
    {
      entry.m_bInUse = true;
      entry.m_pRasterizerView->SetResolution(uiWidth, uiHeight, fAspectRatio);
      return entry.m_pRasterizerView.Borrow();
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_pRasterizerView = EZ_DEFAULT_NEW(ezRasterizerView, ezRasterizerImplementation::Auto);
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
