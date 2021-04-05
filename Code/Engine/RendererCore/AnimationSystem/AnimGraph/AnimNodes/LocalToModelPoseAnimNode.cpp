#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLocalToModelPoseAnimNode, 1, ezRTTIDefaultAllocator<ezLocalToModelPoseAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Pose Processing"),
    new ezColorAttribute(ezColor::FireBrick),
    new ezTitleAttribute("Local To Model Space"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLocalToModelPoseAnimNode::ezLocalToModelPoseAnimNode() = default;
ezLocalToModelPoseAnimNode::~ezLocalToModelPoseAnimNode() = default;

ezResult ezLocalToModelPoseAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLocalToModelPoseAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLocalToModelPoseAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_ModelPosePin.IsConnected())
    return;

  auto pLocalPose = m_LocalPosePin.GetPose(graph);
  if (pLocalPose == nullptr)
    return;

  if (m_pModelTransform == nullptr)
  {
    m_pModelTransform = graph.AllocateModelTransforms(*pSkeleton);
  }

  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  auto& msTrans = m_pModelTransform->m_ModelTransforms;
  msTrans.SetCountUninitialized(pOzzSkeleton->num_joints());


  ozz::animation::LocalToModelJob job;
  job.input = make_span(pLocalPose->m_ozzLocalTransforms);
  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(begin(msTrans)), reinterpret_cast<ozz::math::Float4x4*>(end(msTrans)));
  job.skeleton = pOzzSkeleton;
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  m_pModelTransform->m_bUseRootMotion = pLocalPose->m_bUseRootMotion;
  m_pModelTransform->m_vRootMotion = pLocalPose->m_vRootMotion;

  m_ModelPosePin.SetPose(graph, m_pModelTransform);
}
