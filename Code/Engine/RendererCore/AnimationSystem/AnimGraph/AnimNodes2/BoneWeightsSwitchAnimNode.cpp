#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/BoneWeightsSwitchAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSwitchBoneWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezSwitchBoneWeightsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TransitionDuration", m_TransitionDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromMilliseconds(200))),
    EZ_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("WeightsCount", m_uiWeightsCount)->AddAttributes(new ezNoTemporaryTransactionsAttribute(), new ezDynamicPinAttribute(), new ezDefaultValueAttribute(2)),
    EZ_ARRAY_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new ezHiddenAttribute(), new ezDynamicPinAttribute("WeightsCount")),
    EZ_MEMBER_PROPERTY("OutWeights", m_OutWeights)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Weights"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Teal)),
    new ezTitleAttribute("Bone Weights Switch"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSwitchBoneWeightsAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_TransitionDuration;
  stream << m_uiWeightsCount;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InWeights));
  EZ_SUCCEED_OR_RETURN(m_OutWeights.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSwitchBoneWeightsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  if (version >= 2)
  {
    stream >> m_TransitionDuration;
  }

  stream >> m_uiWeightsCount;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_InWeights));
  EZ_SUCCEED_OR_RETURN(m_OutWeights.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSwitchBoneWeightsAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutWeights.IsConnected() || !m_InIndex.IsConnected() || m_InWeights.IsEmpty())
    return;

  ezHybridArray<const ezAnimGraphBoneWeightsInputPin*, 12> pPins;
  for (ezUInt32 i = 0; i < m_InWeights.GetCount(); ++i)
  {
    pPins.PushBack(&m_InWeights[i]);
  }

  // duplicate pin connections to fill up holes
  for (ezUInt32 i = 1; i < pPins.GetCount(); ++i)
  {
    if (!pPins[i]->IsConnected())
      pPins[i] = pPins[i - 1];
  }
  for (ezUInt32 i = pPins.GetCount(); i > 1; --i)
  {
    if (!pPins[i - 2]->IsConnected())
      pPins[i - 2] = pPins[i - 1];
  }

  if (pPins.IsEmpty() || !pPins[0]->IsConnected())
  {
    // this can only be the case if no pin is connected, at all
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const ezInt8 iDstIdx = ezMath::Clamp<ezInt8>((ezInt8)m_InIndex.GetNumber(ref_graph, 0), 0, pPins.GetCount() - 1);

  if (pInstance->m_iTransitionToIndex < 0)
  {
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_iTransitionFromIndex = iDstIdx;
  }

  pInstance->m_TransitionTime += tDiff;

  if (iDstIdx != pInstance->m_iTransitionToIndex)
  {
    if (iDstIdx == pInstance->m_iTransitionFromIndex)
    {
      // if we transition back to the previous index, just reverse the transition
      pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
      pInstance->m_iTransitionToIndex = iDstIdx;
      pInstance->m_TransitionTime = ezMath::Max(ezTime::MakeZero(), m_TransitionDuration - pInstance->m_TransitionTime);
    }
    else if (pInstance->m_TransitionTime < m_TransitionDuration * 0.5)
    {
      // if we are still in the first half of the transition, switch the target index,
      // but keep the source index and transition time
      pInstance->m_iTransitionToIndex = iDstIdx;
    }
    else
    {
      // otherwise just start a new transition from the current target to the new target
      pInstance->m_TransitionTime = ezTime::MakeZero();
      pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
      pInstance->m_iTransitionToIndex = iDstIdx;
    }
  }

  if (pInstance->m_TransitionTime >= m_TransitionDuration)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
  }

  EZ_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex < (ezInt32)pPins.GetCount(), "Invalid pose index");

  const ezInt8 iTransitionFromIndex = pInstance->m_iTransitionFromIndex;
  const ezInt8 iTransitionToIndex = pInstance->m_iTransitionToIndex;

  if (iTransitionFromIndex == iTransitionToIndex)
  {
    m_OutWeights.SetWeights(ref_graph, pPins[iTransitionToIndex]->GetWeights(ref_controller, ref_graph));
  }
  else
  {
    const float fLerp0 = (float)ezMath::Clamp(pInstance->m_TransitionTime.GetSeconds() / m_TransitionDuration.GetSeconds(), 0.0, 1.0);
    const float fLerp = static_cast<float>(ezMath::GetCurveValue_EaseInOutCubic(fLerp0));

    auto pWeights0 = pPins[iTransitionFromIndex]->GetWeights(ref_controller, ref_graph);
    auto pWeights1 = pPins[iTransitionToIndex]->GetWeights(ref_controller, ref_graph);
    if (pWeights0 == nullptr || pWeights1 == nullptr)
      return;
    if (pWeights0->m_pSharedBoneWeights == nullptr || pWeights1->m_pSharedBoneWeights == nullptr)
      return;

    if (pInstance->m_pBlendedBoneWeights == nullptr)
    {
      static ezAtomicInteger32 iCounter = 0;
      ezStringBuilder sTmpName;
      sTmpName.SetFormat("SwitchBlend_Tmp_{}", iCounter.PostIncrement());

      pInstance->m_pBlendedBoneWeights = ref_controller.CreateBoneWeights(sTmpName, *pSkeleton, [](ezAnimGraphSharedBoneWeights& ref_bw) {});
    }

    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    // Fill out pInstance->m_pBlendedBoneWeights with the lerp between pWeights0 and pWeights1
    const auto& weights0 = pWeights0->m_pSharedBoneWeights->m_Weights;
    const auto& weights1 = pWeights1->m_pSharedBoneWeights->m_Weights;
    auto& blendedWeights = pInstance->m_pBlendedBoneWeights->m_Weights;

    const ezUInt32 numWeights = ezMath::Min(weights0.GetCount(), weights1.GetCount());
    blendedWeights.SetCount(numWeights);

    const ozz::math::SimdFloat4 lerp1 = ozz::math::simd_float4::Load1(1.0f - fLerp);
    const ozz::math::SimdFloat4 lerp2 = ozz::math::simd_float4::Load1(fLerp);

    // lerp each component of SimdFloat4
    for (ezUInt32 i = 0; i < numWeights; ++i)
    {
      const ozz::math::SimdFloat4& w0 = weights0[i];
      const ozz::math::SimdFloat4& w1 = weights1[i];

      blendedWeights[i] = ozz::math::MAdd(w1, lerp2, ozz::math::MAdd(w0, lerp1, ozz::math::simd_float4::zero()));
    }

    ezAnimGraphPinDataBoneWeights* pPinData = ref_controller.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = ezMath::Lerp(pWeights0->m_fOverallWeight, pWeights1->m_fOverallWeight, fLerp);
    pPinData->m_pSharedBoneWeights = pInstance->m_pBlendedBoneWeights.Borrow();

    m_OutWeights.SetWeights(ref_graph, pPinData);
  }
}

bool ezSwitchBoneWeightsAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_BoneWeightsSwitchAnimNode);
