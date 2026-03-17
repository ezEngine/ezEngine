#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRenderSortingFunctions, 1)
  EZ_ENUM_CONSTANTS(ezRenderSortingFunctions::ByRenderDataThenFrontToBack, ezRenderSortingFunctions::BackToFrontThenByRenderData, ezRenderSortingFunctions::ByDepthOffsetOnly)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  EZ_ALWAYS_INLINE ezUInt32 CalculateTypeHash(const ezRenderData* pRenderData)
  {
    ezUInt32 uiTypeHash = ezHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  template <ezUInt32 Bits>
  EZ_ALWAYS_INLINE ezUInt64 NormalizeDistance(float fDistance, float fMaxDistance)
  {
    const float fNormalizedDistance = ezMath::Saturate(fDistance / fMaxDistance);
    return static_cast<ezUInt64>(fNormalizedDistance * static_cast<float>(EZ_BIT(Bits) - 1));
  }

  template <ezUInt32 Bits>
  EZ_FORCE_INLINE ezUInt64 CalculateDistance(const ezRenderData* pRenderData, const ezCamera& camera)
  {
    ///\todo far-plane is not enough to normalize distance
    const float fDistance = (camera.GetPosition() - pRenderData->m_vGlobalPosition).GetLength() + pRenderData->m_fSortingDepthOffset;
    return NormalizeDistance<Bits>(fDistance, camera.GetFarPlane());
  }
} // namespace

// static
ezUInt64 ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc(const ezRenderData* pRenderData, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const ezUInt64 uiDistance = CalculateDistance<16>(pRenderData, camera);

  const ezUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
ezUInt64 ezRenderSortingFunctions::BackToFrontThenByRenderDataFunc(const ezRenderData* pRenderData, const ezCamera& camera)
{
  const ezUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const ezUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const ezUInt64 uiInvDistance = 0xFFFF - CalculateDistance<16>(pRenderData, camera);

  const ezUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}

// static
ezUInt64 ezRenderSortingFunctions::ByDepthOffsetOnlyFunc(const ezRenderData* pRenderData, const ezCamera& camera)
{
  const float fMidDistance = camera.GetFarPlane() * 0.5f;
  const float fDistance = fMidDistance + pRenderData->m_fSortingDepthOffset;
  const ezUInt64 uiInvDistance = 0xFFFFFFFF - NormalizeDistance<32>(fDistance, camera.GetFarPlane());

  return uiInvDistance;
}

// static
ezRenderSortingFunctions::Func ezRenderSortingFunctions::GetFunction(Enum sortingFunction)
{
  switch (sortingFunction)
  {
    case ByRenderDataThenFrontToBack:
      return &ByRenderDataThenFrontToBackFunc;
    case BackToFrontThenByRenderData:
      return &BackToFrontThenByRenderDataFunc;
    case ByDepthOffsetOnly:
      return &ByDepthOffsetOnlyFunc;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return nullptr;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);

