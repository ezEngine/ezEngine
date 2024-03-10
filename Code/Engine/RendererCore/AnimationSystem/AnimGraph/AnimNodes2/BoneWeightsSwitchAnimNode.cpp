#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/BoneWeightsSwitchAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSwitchBoneWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezSwitchBoneWeightsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
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
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiWeightsCount;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InWeights));
  EZ_SUCCEED_OR_RETURN(m_OutWeights.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSwitchBoneWeightsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

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

  const ezInt32 iIndex = ezMath::Clamp((ezInt32)m_InIndex.GetNumber(ref_graph), 0, (ezInt32)m_InWeights.GetCount() - 1);

  if (!m_InWeights[iIndex].IsConnected())
    return;

  m_OutWeights.SetWeights(ref_graph, m_InWeights[iIndex].GetWeights(ref_controller, ref_graph));
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_BoneWeightsSwitchAnimNode);
