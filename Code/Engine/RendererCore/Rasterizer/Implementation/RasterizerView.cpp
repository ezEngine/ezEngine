#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

ezCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, ezCVarFlags::Default, "Max resolution for occlusion buffers.");
ezCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, ezCVarFlags::Default, "Max number of occluders to rasterize per frame.");

ezRasterizerView::ezRasterizerView() = default;
ezRasterizerView::~ezRasterizerView() = default;

void ezRasterizerView::SetResolution(ezUInt32 width, ezUInt32 height, float fAspectRatio)
{
  if (m_uiResolutionX != width || m_uiResolutionY != height)
  {
    m_uiResolutionX = width;
    m_uiResolutionY = height;

    m_pRasterizer = EZ_DEFAULT_NEW(Rasterizer, width, height);
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void ezRasterizerView::BeginScene()
{
  EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");

  EZ_PROFILE_SCOPE("Occlusion::Clear");

  m_pRasterizer->clear();
  m_bAnyOccludersRasterized = false;
}

void ezRasterizerView::ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const
{
  EZ_PROFILE_SCOPE("Occlusion::ReadFrame");

  EZ_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
  EZ_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
}

void ezRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("Occlusion::RasterizeScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void ezRasterizerView::RasterizeObjects(ezUInt32 uiMaxObjects)
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)

  EZ_PROFILE_SCOPE("Occlusion::RasterizeObjects");

  for (const Instance& inst : m_Instances)
  {
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
#endif
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

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void ezRasterizerView::SortObjectsFrontToBack()
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  EZ_PROFILE_SCOPE("Occlusion::SortObjects");

  const ezVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2) {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
#endif
}

bool ezRasterizerView::IsVisible(const ezSimdBBox& aabb) const
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  EZ_PROFILE_SCOPE("Occlusion::IsVisible");

  ezSimdVec4f vmin = aabb.m_Min;
  ezSimdVec4f vmax = aabb.m_Max;

  // ezSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#else
  return true;
#endif
}

ezRasterizerView* ezRasterizerViewPool::GetRasterizerView(ezUInt32 width, ezUInt32 height, float fAspectRatio)
{
  EZ_PROFILE_SCOPE("Occlusion::GetViewFromPool");

  EZ_LOCK(m_Mutex);

  const float divX = (float)width / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)height / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div = ezMath::Max(divX, divY);

  if (div > 1.0)
  {
    width = (ezUInt32)(width / div);
    height = (ezUInt32)(height / div);
  }

  width = ezMath::RoundDown(width, 8);
  height = ezMath::RoundDown(height, 8);

  width = ezMath::Clamp<ezUInt32>(width, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  height = ezMath::Clamp<ezUInt32>(height, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_RasterizerView.GetResolutionX() == width && entry.m_RasterizerView.GetResolutionY() == height)
    {
      entry.m_bInUse = true;
      entry.m_RasterizerView.SetResolution(width, height, fAspectRatio);
      return &entry.m_RasterizerView;
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_RasterizerView.SetResolution(width, height, fAspectRatio);
  ne.m_bInUse = true;

  return &ne.m_RasterizerView;
}

void ezRasterizerViewPool::ReturnRasterizerView(ezRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  EZ_PROFILE_SCOPE("Occlusion::ReturnViewToPool");

  pView->SetCamera(nullptr);

  EZ_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (&entry.m_RasterizerView == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerView);
