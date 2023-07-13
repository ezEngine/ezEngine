#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPoseResultAnimNode, 1, ezRTTIDefaultAllocator<ezPoseResultAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FadeDuration", m_FadeDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(200)), new ezClampValueAttribute(ezTime::Zero(), ezTime::Seconds(10))),
    EZ_MEMBER_PROPERTY("InPose", m_InPose)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InTargetWeight", m_InTargetWeight)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InFadeDuration", m_InFadeDuration)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutOnFadedOut", m_OutOnFadedOut)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutOnFadedIn", m_OutOnFadedIn)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutCurrentWeight", m_OutCurrentWeight)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Output"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Grape)),
    new ezTitleAttribute("Pose Result"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPoseResultAnimNode::ezPoseResultAnimNode() = default;
ezPoseResultAnimNode::~ezPoseResultAnimNode() = default;

ezResult ezPoseResultAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_FadeDuration;

  EZ_SUCCEED_OR_RETURN(m_InPose.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InTargetWeight.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InFadeDuration.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFadedOut.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFadedIn.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutCurrentWeight.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPoseResultAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_FadeDuration;

  EZ_SUCCEED_OR_RETURN(m_InPose.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InTargetWeight.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InFadeDuration.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InWeights.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFadedOut.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFadedIn.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutCurrentWeight.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezPoseResultAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InPose.IsConnected())
    return;

  InstanceData* pInstance = graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bWasInterpolating = pInstance->m_PlayTime < pInstance->m_EndTime;

  float fCurrentWeight = 1.0f;
  pInstance->m_PlayTime += tDiff;

  if (pInstance->m_PlayTime >= pInstance->m_EndTime)
  {
    fCurrentWeight = pInstance->m_fEndWeight;

    if (bWasInterpolating && fCurrentWeight <= 0.0f)
    {
      m_OutOnFadedOut.SetTriggered(graph);
    }
    if (bWasInterpolating && fCurrentWeight >= 1.0f)
    {
      m_OutOnFadedIn.SetTriggered(graph);
    }
  }
  else
  {
    const float f = (float)(pInstance->m_PlayTime.GetSeconds() / pInstance->m_EndTime.GetSeconds());
    fCurrentWeight = ezMath::Lerp(pInstance->m_fStartWeight, pInstance->m_fEndWeight, f);
  }

  const float fNewTargetWeight = m_InTargetWeight.GetNumber(graph, 1.0f);

  if (pInstance->m_fEndWeight != fNewTargetWeight)
  {
    pInstance->m_fStartWeight = fCurrentWeight;
    pInstance->m_fEndWeight = fNewTargetWeight;
    pInstance->m_PlayTime = ezTime::Zero();
    pInstance->m_EndTime = ezTime::Seconds(m_InFadeDuration.GetNumber(graph, m_FadeDuration.GetSeconds()));
  }

  m_OutCurrentWeight.SetNumber(graph, fCurrentWeight);

  if (fCurrentWeight <= 0.0f)
    return;

  if (auto pCurrentLocalTransforms = m_InPose.GetPose(ref_controller, graph))
  {
    if (pCurrentLocalTransforms->m_CommandID != ezInvalidIndex)
    {
      ezAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID = pCurrentLocalTransforms->m_CommandID;
      pLocalTransforms->m_pWeights = m_InWeights.GetWeights(ref_controller, graph);
      pLocalTransforms->m_fOverallWeight = pCurrentLocalTransforms->m_fOverallWeight * fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = pCurrentLocalTransforms->m_bUseRootMotion;
      pLocalTransforms->m_vRootMotion = pCurrentLocalTransforms->m_vRootMotion;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
}

bool ezPoseResultAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}
