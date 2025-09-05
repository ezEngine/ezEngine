#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SwitchPoseAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSwitchPoseAnimNode, 1, ezRTTIDefaultAllocator<ezSwitchPoseAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TransitionDuration", m_TransitionDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromMilliseconds(200))),
    EZ_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new ezNoTemporaryTransactionsAttribute(), new ezDynamicPinAttribute(), new ezDefaultValueAttribute(2)),
    EZ_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new ezHiddenAttribute(), new ezDynamicPinAttribute("PosesCount")),
    EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Pose Blending"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Yellow)),
    new ezTitleAttribute("Pose Switch"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSwitchPoseAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_TransitionDuration;
  stream << m_uiPosesCount;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSwitchPoseAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_TransitionDuration;
  stream >> m_uiPosesCount;

  EZ_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSwitchPoseAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InIndex.IsConnected())
    return;

  ezHybridArray<const ezAnimGraphLocalPoseInputPin*, 12> pPins;
  for (ezUInt32 i = 0; i < m_InPoses.GetCount(); ++i)
  {
    pPins.PushBack(&m_InPoses[i]);
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

  ezInt8 iTransitionFromIndex = pInstance->m_iTransitionFromIndex;
  ezInt8 iTransitionToIndex = pInstance->m_iTransitionToIndex;

  if (pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph) == nullptr)
  {
    // if the 'from' pose already stopped, just jump to the 'to' pose
    iTransitionFromIndex = iTransitionToIndex;
  }

  if (iTransitionFromIndex == iTransitionToIndex)
  {
    const ezAnimGraphLocalPoseInputPin* pPinToForward = pPins[iTransitionToIndex];
    ezAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);

    if (pDataToForward == nullptr)
      return;

    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();
    pLocalTransforms->m_CommandID = pDataToForward->m_CommandID;
    pLocalTransforms->m_pWeights = pDataToForward->m_pWeights;
    pLocalTransforms->m_fOverallWeight = pDataToForward->m_fOverallWeight;
    pLocalTransforms->m_vRootMotion = pDataToForward->m_vRootMotion;
    pLocalTransforms->m_bUseRootMotion = pDataToForward->m_bUseRootMotion;

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
  else
  {
    auto pPose0 = pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[iTransitionToIndex]->GetPose(ref_controller, ref_graph);

    if (pPose0 == nullptr || pPose1 == nullptr)
      return;

    ezAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp0 = (float)ezMath::Clamp(pInstance->m_TransitionTime.GetSeconds() / m_TransitionDuration.GetSeconds(), 0.0, 1.0);
    const float fLerp = static_cast<float>(ezMath::GetCurveValue_EaseInOutCubic(fLerp0));

    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pPose0->m_CommandID;
    cmd.m_Inputs[1] = pPose1->m_CommandID;

    pPinData->m_CommandID = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = pPose0->m_bUseRootMotion || pPose1->m_bUseRootMotion;
    pPinData->m_vRootMotion = ezMath::Lerp(pPose0->m_vRootMotion, pPose1->m_vRootMotion, fLerp);

    m_OutPose.SetPose(ref_graph, pPinData);
  }
}

bool ezSwitchPoseAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SwitchPoseAnimNode);
