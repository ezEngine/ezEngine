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
  if (m_Objects.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("Occlusion::RasterizeScene");

  SortObjectsFrontToBack();

  ApplyModelViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Objects.Clear();
}

void ezRasterizerView::RasterizeObjects(ezUInt32 uiMaxObjects)
{
  EZ_PROFILE_SCOPE("Occlusion::RasterizeObjects");

  for (const ezRasterizerObject* pObj : m_Objects)
  {
    bool bNeedsClipping;
    const Occluder& occluder = pObj->GetInternalOccluder();

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

void ezRasterizerView::ApplyModelViewProjectionMatrix()
{
  ezMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection);

  const ezMat4 mView = m_pCamera->GetViewMatrix();
  const ezMat4 mMVP = mProjection * mView;

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void ezRasterizerView::SortObjectsFrontToBack()
{
  EZ_PROFILE_SCOPE("Occlusion::SortObjects");

  ezSimdVec4f camPos;
  camPos.Load<3>(m_pCamera->GetCenterPosition().GetData());
  camPos.SetW(1);

  m_Objects.Sort([&](const ezRasterizerObject* o1, const ezRasterizerObject* o2)
    {
        __m128 dist1 = _mm_sub_ps(o1->GetInternalOccluder().m_center, camPos.m_v);
        __m128 dist2 = _mm_sub_ps(o2->GetInternalOccluder().m_center, camPos.m_v);

        return _mm_comilt_ss(_mm_dp_ps(dist1, dist1, 0x7f), _mm_dp_ps(dist2, dist2, 0x7f)); });
}

bool ezRasterizerView::IsVisible(const ezBoundingBox& aabb) const
{
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  EZ_PROFILE_SCOPE("Occlusion::IsVisible");

  const ezSimdVec4f vMin = ezSimdConversion::ToVec4(aabb.m_vMin.GetAsPositionVec4());
  const ezSimdVec4f vMax = ezSimdConversion::ToVec4(aabb.m_vMax.GetAsPositionVec4());

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vMin.m_v, vMax.m_v, needsClipping);
}

bool ezRasterizerView::IsVisible(const ezRasterizerObject& object) const
{
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  EZ_PROFILE_SCOPE("Occlusion::IsVisible");

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(object.GetInternalOccluder().m_boundsMin, object.GetInternalOccluder().m_boundsMax, needsClipping);
}

bool ezRasterizerView::IsVisible(const ezSimdBBox& aabb) const
{
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
