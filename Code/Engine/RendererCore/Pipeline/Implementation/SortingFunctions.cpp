#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

namespace
{
  EZ_FORCE_INLINE ezUInt32 CalculateTypeHash(const ezRenderData* pRenderData)
  {
    ezUInt32 uiTypeHash = pRenderData->GetDynamicRTTI()->GetTypeNameHash();
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  EZ_FORCE_INLINE ezUInt32 CalculateDistance(const ezRenderData* pRenderData, const ezCamera& camera)
  {
    ///\todo might need to use projected length instead?
    const float fDistance = (camera.GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
    const float fNormalizedDistance = ezMath::Min(fDistance / camera.GetFarPlane(), 1.0f);
    return static_cast<ezUInt32>(fNormalizedDistance * 65535.0f);
  }
}

//static
ezUInt64 ezRenderSortingFunctions::ByRenderDataThenFrontToBack(const ezRenderData* pRenderData, ezUInt32 uiRenderDataSortingKey, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const ezUInt64 uiDistance = CalculateDistance(pRenderData, camera);
  
  const ezUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;  
  return uiSortingKey;
}

//static
ezUInt64 ezRenderSortingFunctions::BackToFrontThenByRenderData(const ezRenderData* pRenderData, ezUInt32 uiRenderDataSortingKey, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const ezUInt64 uiInvDistance = 0xFFFF - CalculateDistance(pRenderData, camera);

  const ezUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);

