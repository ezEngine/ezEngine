#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/BoneWeightsSwitchAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSwitchBoneWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezSwitchBoneWeightsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InWeights0", m_InWeights0)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InWeights1", m_InWeights1)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InWeights2", m_InWeights2)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InWeights3", m_InWeights3)->AddAttributes(new ezHiddenAttribute()),
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

  EZ_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights2.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights3.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutWeights.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSwitchBoneWeightsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights2.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights3.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutWeights.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSwitchBoneWeightsAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_OutWeights.IsConnected() || !m_InIndex.IsConnected())
    return;

  const ezInt32 iIndex = ezMath::Clamp((ezInt32)m_InIndex.GetNumber(graph), 0, 3);

  ezAnimGraphBoneWeightsInputPin* pPin[4] = {&m_InWeights0, &m_InWeights1, &m_InWeights2, &m_InWeights3};

  if (!pPin[iIndex]->IsConnected())
    return;

  m_OutWeights.SetWeights(graph, pPin[iIndex]->GetWeights(graph));
}
