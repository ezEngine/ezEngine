#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//// clang-format off
// EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLocalToModelPoseAnimNode, 1, ezRTTIDefaultAllocator<ezLocalToModelPoseAnimNode>)
//{
//   EZ_BEGIN_PROPERTIES
//   {
//     EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute),
//     EZ_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new ezHiddenAttribute),
//   }
//   EZ_END_PROPERTIES;
//   EZ_BEGIN_ATTRIBUTES
//   {
//     new ezCategoryAttribute("Pose Processing"),
//     new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
//     new ezTitleAttribute("Local To Model Space"),
//   }
//   EZ_END_ATTRIBUTES;
// }
// EZ_END_DYNAMIC_REFLECTED_TYPE;
//// clang-format on
//
// ezLocalToModelPoseAnimNode::ezLocalToModelPoseAnimNode() = default;
// ezLocalToModelPoseAnimNode::~ezLocalToModelPoseAnimNode() = default;
//
// ezResult ezLocalToModelPoseAnimNode::SerializeNode(ezStreamWriter& stream) const
//{
//  stream.WriteVersion(1);
//
//  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));
//
//  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
//
//  return EZ_SUCCESS;
//}
//
// ezResult ezLocalToModelPoseAnimNode::DeserializeNode(ezStreamReader& stream)
//{
//  stream.ReadVersion(1);
//
//  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));
//
//  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
//
//  return EZ_SUCCESS;
//}
//
// void ezLocalToModelPoseAnimNode::Step(ezAnimGraphExecutor& executor, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
//{
//  if (!m_LocalPosePin.IsConnected() || !m_ModelPosePin.IsConnected())
//    return;
//
//  auto pLocalPose = m_LocalPosePin.GetPose(graph);
//  if (pLocalPose == nullptr)
//    return;
//
//  ezAnimGraphPinDataModelTransforms* pModelTransform = graph.AddPinDataModelTransforms();
//
//  if (pLocalPose->m_bUseRootMotion)
//  {
//    pModelTransform->m_bUseRootMotion = true;
//    pModelTransform->m_vRootMotion = pLocalPose->m_vRootMotion;
//  }
//
//  auto& cmd = graph.GetPoseGenerator().AllocCommandLocalToModelPose();
//  cmd.m_Inputs.PushBack(m_LocalPosePin.GetPose(graph)->m_CommandID);
//
//  pModelTransform->m_CommandID = cmd.GetCommandID();
//
//  m_ModelPosePin.SetPose(graph, pModelTransform);
//}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
