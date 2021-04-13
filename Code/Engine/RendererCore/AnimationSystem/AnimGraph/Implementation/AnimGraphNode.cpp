#include <RendererCorePCH.h>

#include <AnimationSystem/AnimGraph/AnimGraph.h>
#include <AnimationSystem/AnimationClipResource.h>
#include <AnimationSystem/SkeletonResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

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
