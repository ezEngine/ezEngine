#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

namespace
{
  EZ_FORCE_INLINE ezUInt16 CalculateTypeHash(const ezRenderData* pRenderData)
  {
    ezUInt32 uiTypeHash = pRenderData->GetDynamicRTTI()->GetTypeNameHash();
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  EZ_FORCE_INLINE ezUInt16 CalculateDistance(const ezRenderData* pRenderData, const ezCamera& camera)
  {
    const float fDistance = (camera.GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
    const float fNormalizedDistance = ezMath::Min(fDistance / camera.GetFarPlane(), 1.0f);
    return static_cast<ezUInt16>(fNormalizedDistance * 65535.0f);
  }
}

//static
ezUInt64 ezRenderSortingFunctions::ByBatchThenFrontToBack(const ezRenderData* pRenderData, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiBatchId = pRenderData->m_uiBatchId;
  const ezUInt64 uiDistance = CalculateDistance(pRenderData, camera);
  
  const ezUInt64 uiSortingKey = (uiTypeHash << 48) | (uiBatchId << 16) | uiDistance;  
  return uiSortingKey;
}

//static
ezUInt64 ezRenderSortingFunctions::BackToFrontThenByBatch(const ezRenderData* pRenderData, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiBatchId = pRenderData->m_uiBatchId;
  const ezUInt64 uiInvDistance = 0xFFFF - CalculateDistance(pRenderData, camera);

  const ezUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiBatchId;
  return uiSortingKey;
}

