#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <ozz/animation/runtime/sampling_job.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNode, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CustomTitle", GetCustomNodeTitle, SetCustomNodeTitle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphNode::ezAnimGraphNode() = default;
ezAnimGraphNode::~ezAnimGraphNode() = default;

ezResult ezAnimGraphNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  stream << m_CustomNodeTitle;

  return EZ_SUCCESS;
}

ezResult ezAnimGraphNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  stream >> m_CustomNodeTitle;

  return EZ_SUCCESS;
}

void ezAnimGraphNode::SampleAnimation(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache, const ozz::animation::Animation* pOzzAnimation, float lookupPos)
{
  ozz::animation::SamplingJob job;
  job.animation = pOzzAnimation;
  job.cache = &cache.m_ozzSamplingCache;
  job.ratio = lookupPos;
  job.output = make_span(transform.m_ozzLocalTransforms);
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void ezAnimGraphNode::SampleAnimation(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache, const ezAnimationClipResource& animClip, const ezSkeletonResource& skeleton, ezTime lookupTime)
{
  ozz::animation::SamplingJob job;
  job.animation = &animClip.GetDescriptor().GetMappedOzzAnimation(skeleton);
  job.cache = &cache.m_ozzSamplingCache;
  job.ratio = lookupTime.AsFloatInSeconds() / animClip.GetDescriptor().GetDuration().AsFloatInSeconds();
  job.output = make_span(transform.m_ozzLocalTransforms);
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void ezAnimGraphNode::LerpAnimations(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache0, const ezAnimationClipResource& animClip0, ezTime lookupTime0, ezAnimGraphLocalTransforms& tempTransform0, ezAnimGraphSamplingCache& cache1, const ezAnimationClipResource& animClip1, ezTime lookupTime1, ezAnimGraphLocalTransforms& tempTransform1, const ezSkeletonResource& skeleton, float fLerpFactor)
{
  SampleAnimation(tempTransform0, cache0, animClip0, skeleton, lookupTime0);
  SampleAnimation(tempTransform1, cache1, animClip1, skeleton, lookupTime1);

  ozz::animation::BlendingJob::Layer bl[2];

  bl[0].transform = make_span(tempTransform0.m_ozzLocalTransforms);
  bl[1].transform = make_span(tempTransform1.m_ozzLocalTransforms);

  bl[0].weight = 1.0f - fLerpFactor;
  bl[1].weight = fLerpFactor;

  ozz::animation::BlendingJob job;
  job.threshold = 0.1f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(bl, bl + 2);
  job.bind_pose = skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_bind_poses();
  job.output = make_span(transform.m_ozzLocalTransforms);
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

int ezAnimGraphNode::CrossfadeAnimations(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache0, const ezAnimationClipResource& animClip0, ezAnimGraphLocalTransforms& tempTransform0, ezAnimGraphSamplingCache& cache1, const ezAnimationClipResource& animClip1, ezAnimGraphLocalTransforms& tempTransform1, const ezSkeletonResource& skeleton, ezTime lookupTime, ezTime crossfadeDuration)
{
  const ezTime duration0 = animClip0.GetDescriptor().GetDuration();

  if (lookupTime <= duration0 - crossfadeDuration)
  {
    SampleAnimation(transform, cache0, animClip0, skeleton, lookupTime);
    return -1;
  }

  const ezTime lookup2 = lookupTime - (duration0 - crossfadeDuration);

  if (lookupTime >= duration0)
  {
    SampleAnimation(transform, cache1, animClip1, skeleton, lookup2);

    const ezTime duration1 = animClip1.GetDescriptor().GetDuration();

    if (lookupTime >= duration0 + duration1 - crossfadeDuration)
      return 2;

    return 1;
  }

  float fLerp = (duration0 - lookupTime).AsFloatInSeconds() / crossfadeDuration.AsFloatInSeconds();
  fLerp = 1.0f - fLerp;
  //fLerp = 1.0f - ezMath::Square(fLerp);

  LerpAnimations(transform, cache0, animClip0, lookupTime, tempTransform0, cache1, animClip1, lookup2, tempTransform1, skeleton, fLerp);
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAnimRampUpDown, ezNoBase, 1, ezRTTIDefaultAllocator<ezAnimRampUpDown>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RampUp", m_RampUp),
    EZ_MEMBER_PROPERTY("RampDown", m_RampDown),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezAnimRampUpDown::Serialize(ezStreamWriter& stream) const
{
  stream << m_RampUp;
  stream << m_RampDown;

  return EZ_SUCCESS;
}

ezResult ezAnimRampUpDown::Deserialize(ezStreamReader& stream)
{
  stream >> m_RampUp;
  stream >> m_RampDown;

  return EZ_SUCCESS;
}

void ezAnimRampUpDown::RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, ezTime tDiff) const
{
  if (inout_fWeight < fTargetWeight)
  {
    inout_fWeight += tDiff.AsFloatInSeconds() / m_RampUp.AsFloatInSeconds();
    inout_fWeight = ezMath::Min(inout_fWeight, fTargetWeight);
  }
  else if (inout_fWeight > fTargetWeight)
  {
    inout_fWeight -= tDiff.AsFloatInSeconds() / m_RampDown.AsFloatInSeconds();
    inout_fWeight = ezMath::Max(0.0f, inout_fWeight);
  }
}
