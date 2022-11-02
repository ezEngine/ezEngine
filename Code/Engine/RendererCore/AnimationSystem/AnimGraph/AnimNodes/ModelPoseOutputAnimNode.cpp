#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ModelPoseOutputAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezModelPoseOutputAnimNode, 1, ezRTTIDefaultAllocator<ezModelPoseOutputAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("RotateZ", m_RotateZPin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Output"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Grape)),
    new ezTitleAttribute("Output"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezModelPoseOutputAnimNode::ezModelPoseOutputAnimNode() = default;
ezModelPoseOutputAnimNode::~ezModelPoseOutputAnimNode() = default;

ezResult ezModelPoseOutputAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RotateZPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezModelPoseOutputAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RotateZPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezModelPoseOutputAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  ezVec3 rootMotion = ezVec3::ZeroVector();
  ezAngle rootRotationX;
  ezAngle rootRotationY;
  ezAngle rootRotationZ;

  if (m_ModelPosePin.IsConnected())
  {
    if (auto pCurrentModelTransforms = m_ModelPosePin.GetPose(graph))
    {
      if (pCurrentModelTransforms->m_CommandID != ezInvalidIndex)
      {
        auto& cmd = graph.GetPoseGenerator().AllocCommandModelPoseToOutput();
        cmd.m_Inputs.PushBack(m_ModelPosePin.GetPose(graph)->m_CommandID);
      }

      if (pCurrentModelTransforms->m_bUseRootMotion)
      {
        rootMotion = pCurrentModelTransforms->m_vRootMotion;
        rootRotationX = pCurrentModelTransforms->m_RootRotationX;
        rootRotationY = pCurrentModelTransforms->m_RootRotationY;
        rootRotationZ = pCurrentModelTransforms->m_RootRotationZ;
      }

      graph.SetOutputModelTransform(pCurrentModelTransforms);
    }
  }

  if (m_RotateZPin.IsConnected())
  {
    const float rotZ = static_cast<float>(m_RotateZPin.GetNumber(graph));
    rootRotationZ += ezAngle::Degree(rotZ);
  }

  graph.SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
}
