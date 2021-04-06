#include <RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/CombinePosesAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCombinePosesAnimNode, 1, ezRTTIDefaultAllocator<ezCombinePosesAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxPoses", m_uiMaxPoses)->AddAttributes(new ezDefaultValueAttribute(8)),
    EZ_MEMBER_PROPERTY("LocalPoses", m_LocalPosesPin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Pose Processing"),
    new ezColorAttribute(ezColor::CornflowerBlue),
    new ezTitleAttribute("Combine Poses"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCombinePosesAnimNode::ezCombinePosesAnimNode() = default;
ezCombinePosesAnimNode::~ezCombinePosesAnimNode() = default;

ezResult ezCombinePosesAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiMaxPoses;

  EZ_SUCCEED_OR_RETURN(m_LocalPosesPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCombinePosesAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiMaxPoses;

  EZ_SUCCEED_OR_RETURN(m_LocalPosesPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCombinePosesAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LocalPosesPin.IsConnected())
    return;

  ezHybridArray<ezAnimGraphLocalTransforms*, 16> pIn;
  m_LocalPosesPin.GetPoses(graph, pIn);

  if (pIn.IsEmpty())
  {
    graph.FreeLocalTransforms(m_pTransforms);
    return;
  }

  if (m_pTransforms == nullptr)
  {
    m_pTransforms = graph.AllocateLocalTransforms(*pSkeleton);
  }

  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();


  m_pTransforms->m_ozzLocalTransforms.resize(pOzzSkeleton->num_soa_joints());
  m_pTransforms->m_vRootMotion.SetZero();

  ezHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;

  float fSummedRootMotionWeight = 0.0f;

  for (ezUInt32 i = 0; i < pIn.GetCount(); ++i)
  {
    if (pIn[i] != nullptr)
    {
      auto& layer = bl.ExpandAndGetRef();
      layer.weight = pIn[i]->m_fOverallWeight;
      layer.transform = make_span(pIn[i]->m_ozzLocalTransforms);

      if (pIn[i]->m_pWeights)
      {
        layer.joint_weights = make_span(pIn[i]->m_pWeights->m_ozzBoneWeights);
        layer.weight *= pIn[i]->m_pWeights->m_fOverallWeight;
      }

      if (pIn[i]->m_bUseRootMotion)
      {
        fSummedRootMotionWeight += layer.weight;
        m_pTransforms->m_vRootMotion += pIn[i]->m_vRootMotion * layer.weight;
        m_pTransforms->m_bUseRootMotion = true;
      }
    }
  }

  if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
  {
    m_pTransforms->m_vRootMotion /= fSummedRootMotionWeight;
  }

  bl.Sort([](auto& lhs, auto& rhs) { return lhs.weight > rhs.weight; });

  // reduce the number of poses to blend to the maximum allowed number
  if (bl.GetCount() > m_uiMaxPoses)
  {
    bl.SetCount(m_uiMaxPoses);
  }

  ozz::animation::BlendingJob job;
  job.threshold = 0.1f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.bind_pose = pOzzSkeleton->joint_bind_poses();
  job.output = make_span(m_pTransforms->m_ozzLocalTransforms);
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  m_LocalPosePin.SetPose(graph, m_pTransforms);
}
