#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/RootMotionAnimNodes.h>

// clang-format off
 EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRootRotationAnimNode, 1, ezRTTIDefaultAllocator<ezRootRotationAnimNode>)
{
   EZ_BEGIN_PROPERTIES
   {
     EZ_MEMBER_PROPERTY("InRotateX", m_InRotateX)->AddAttributes(new ezHiddenAttribute),
     EZ_MEMBER_PROPERTY("InRotateY", m_InRotateY)->AddAttributes(new ezHiddenAttribute),
     EZ_MEMBER_PROPERTY("InRotateZ", m_InRotateZ)->AddAttributes(new ezHiddenAttribute),
   }
   EZ_END_PROPERTIES;
   EZ_BEGIN_ATTRIBUTES
   {
     new ezCategoryAttribute("Output"),
     new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Grape)),
     new ezTitleAttribute("Root Rotation"),
   }
   EZ_END_ATTRIBUTES;
 }
 EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRootRotationAnimNode::ezRootRotationAnimNode() = default;
ezRootRotationAnimNode::~ezRootRotationAnimNode() = default;

ezResult ezRootRotationAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InRotateX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InRotateY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InRotateZ.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezRootRotationAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InRotateX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InRotateY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InRotateZ.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezRootRotationAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  ezVec3 vRootMotion = ezVec3::ZeroVector();
  ezAngle rootRotationX;
  ezAngle rootRotationY;
  ezAngle rootRotationZ;

  ref_controller.GetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);

  if (m_InRotateX.IsConnected())
  {
    rootRotationX += ezAngle::Degree(static_cast<float>(m_InRotateX.GetNumber(ref_graph)));
  }
  if (m_InRotateY.IsConnected())
  {
    rootRotationY += ezAngle::Degree(static_cast<float>(m_InRotateY.GetNumber(ref_graph)));
  }
  if (m_InRotateZ.IsConnected())
  {
    rootRotationZ += ezAngle::Degree(static_cast<float>(m_InRotateZ.GetNumber(ref_graph)));
  }

  ref_controller.SetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
