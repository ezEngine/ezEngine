#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SelectPoseAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectPoseAnimNode, 1, ezRTTIDefaultAllocator<ezSelectPoseAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TransitionDuration", m_TransitionDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(200))),
    EZ_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InPose0", m_InPose0)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InPose1", m_InPose1)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InPose2", m_InPose2)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InPose3", m_InPose3)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Pose Selection"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Yellow)),
    new ezTitleAttribute("Select Pose"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSelectPoseAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_TransitionDuration;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose2.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose3.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSelectPoseAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_TransitionDuration;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose2.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InPose3.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSelectPoseAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InIndex.IsConnected())
    return;

  const ezAnimGraphLocalPoseInputPin* pPins[4] =
    {
      &m_InPose0,
      &m_InPose1,
      &m_InPose2,
      &m_InPose3,
      //
    };

  // duplicate pin connections to fill up holes
  for (ezUInt32 i = 1; i < 4; ++i)
  {
    if (!pPins[i]->IsConnected())
      pPins[i] = pPins[i - 1];
  }
  for (ezUInt32 i = 4; i > 1; --i)
  {
    if (!pPins[i - 2]->IsConnected())
      pPins[i - 2] = pPins[i - 1];
  }

  if (!pPins[0]->IsConnected())
  {
    // this can only be the case if no pin is connected, at all
    return;
  }

  InstanceData* pInstance = graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const ezInt8 iDstIdx = ezMath::Clamp<ezInt8>((ezInt8)m_InIndex.GetNumber(graph, 0), 0, 3);

  if (pInstance->m_iTransitionToIndex < 0)
  {
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_iTransitionFromIndex = iDstIdx;
  }

  pInstance->m_TransitionTime += tDiff;

  if (iDstIdx != pInstance->m_iTransitionToIndex)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_TransitionTime = ezTime::Zero();
  }

  if (pInstance->m_TransitionTime >= m_TransitionDuration)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
  }

  EZ_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex <= 3, "Invalid pose index");
  EZ_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex <= 3, "Invalid pose index");

  ezInt8 iTransitionFromIndex = pInstance->m_iTransitionFromIndex;
  ezInt8 iTransitionToIndex = pInstance->m_iTransitionToIndex;

  if (pPins[iTransitionFromIndex]->GetPose(graph) == nullptr)
  {
    // if the 'from' pose already stopped, just jump to the 'to' pose
    iTransitionFromIndex = iTransitionToIndex;
  }

  if (iTransitionFromIndex == iTransitionToIndex)
  {
    const ezAnimGraphLocalPoseInputPin* pPinToForward = pPins[iTransitionToIndex];
    ezAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(graph);

    if (pDataToForward == nullptr)
      return;

    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();
    pLocalTransforms->m_CommandID = pDataToForward->m_CommandID;
    pLocalTransforms->m_pWeights = pDataToForward->m_pWeights;
    pLocalTransforms->m_fOverallWeight = pDataToForward->m_fOverallWeight;
    pLocalTransforms->m_vRootMotion = pDataToForward->m_vRootMotion;
    pLocalTransforms->m_bUseRootMotion = pDataToForward->m_bUseRootMotion;

    m_OutPose.SetPose(graph, pLocalTransforms);
  }
  else
  {
    auto pPose0 = pPins[iTransitionFromIndex]->GetPose(graph);
    auto pPose1 = pPins[iTransitionToIndex]->GetPose(graph);

    if (pPose0 == nullptr || pPose1 == nullptr)
      return;

    ezAnimGraphPinDataLocalTransforms* pPinData = graph.AddPinDataLocalTransforms();

    const float fLerp = (float)ezMath::Clamp(pInstance->m_TransitionTime.GetSeconds() / m_TransitionDuration.GetSeconds(), 0.0, 1.0);

    auto& cmd = graph.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pPose0->m_CommandID;
    cmd.m_Inputs[1] = pPose1->m_CommandID;

    pPinData->m_CommandID = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = pPose0->m_bUseRootMotion || pPose1->m_bUseRootMotion;
    pPinData->m_vRootMotion = ezMath::Lerp(pPose0->m_vRootMotion, pPose1->m_vRootMotion, fLerp);

    m_OutPose.SetPose(graph, pPinData);
  }
}

bool ezSelectPoseAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}
