#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/LerpPosesAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLerpPosesAnimNode, 1, ezRTTIDefaultAllocator<ezLerpPosesAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Lerp", m_fLerp)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 3.0f)),
    EZ_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new ezNoTemporaryTransactionsAttribute(), new ezDynamicPinAttribute(), new ezDefaultValueAttribute(2)),
    EZ_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new ezHiddenAttribute(), new ezDynamicPinAttribute("PosesCount")),
    EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Pose Blending"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Violet)),
    new ezTitleAttribute("Lerp Poses"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLerpPosesAnimNode::ezLerpPosesAnimNode() = default;
ezLerpPosesAnimNode::~ezLerpPosesAnimNode() = default;

ezResult ezLerpPosesAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fLerp;
  stream << m_uiPosesCount;

  EZ_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLerpPosesAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fLerp;
  stream >> m_uiPosesCount;

  EZ_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLerpPosesAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
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

  const float fIndex = ezMath::Clamp((float)m_InLerp.GetNumber(ref_graph, m_fLerp), 0.0f, (float)pPins.GetCount() - 1.0f);

  if (ezMath::Fraction(fIndex) == 0.0f)
  {
    const ezAnimGraphLocalPoseInputPin* pPinToForward = pPins[(ezInt32)ezMath::Trunc(fIndex)];
    ezAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);

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
    ezAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp = ezMath::Fraction(fIndex);

    auto pPose0 = pPins[(ezInt32)ezMath::Trunc(fIndex)]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[(ezInt32)ezMath::Trunc(fIndex) + 1]->GetPose(ref_controller, ref_graph);

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


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_LerpPosesAnimNode);
