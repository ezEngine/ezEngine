#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class ezSkeletonResource;
class ezGameObject;
class ezAnimGraph;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphSamplingCache;
struct ezAnimGraphLocalTransforms;
struct ezAnimGraphBoneWeights;
class ezAnimationClipResource;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an ezAnimGraph
///
/// These nodes are used to configure which skeletal animations can be played on an object,
/// and how they would be played back exactly.
/// The nodes implement different functionality. For example logic nodes are used to figure out how to play an animation,
/// other nodes then sample and combining animation poses, and yet other nodes can inform the user about events
/// or they write state back to the animation graph's blackboard.
class EZ_RENDERERCORE_DLL ezAnimGraphNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNode, ezReflectedClass);

public:
  ezAnimGraphNode();
  virtual ~ezAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

  static void SampleAnimation(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache, const ozz::animation::Animation* pOzzAnimation, float lookupPos);

  static void SampleAnimation(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache, const ezAnimationClipResource& animClip, const ezSkeletonResource& skeleton, ezTime lookupTime);

  static void LerpAnimations(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache0, const ezAnimationClipResource& animClip0, ezTime lookupTime0, ezAnimGraphLocalTransforms& tempTransform0, ezAnimGraphSamplingCache& cache1, const ezAnimationClipResource& animClip1, ezTime lookupTime1, ezAnimGraphLocalTransforms& tempTransform1, const ezSkeletonResource& skeleton, float fLerpFactor);

  static int CrossfadeAnimations(ezAnimGraphLocalTransforms& transform, ezAnimGraphSamplingCache& cache0, const ezAnimationClipResource& animClip0, ezAnimGraphLocalTransforms& tempTransform0, ezAnimGraphSamplingCache& cache1, const ezAnimationClipResource& animClip1, ezAnimGraphLocalTransforms& tempTransform1, const ezSkeletonResource& skeleton, ezTime lookupTime, ezTime crossfadeDuration);

  const char* GetCustomNodeTitle() const { return m_CustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* sz) { m_CustomNodeTitle.Assign(sz); }

protected:
  friend class ezAnimGraph;

  ezHashedString m_CustomNodeTitle;

  virtual ezResult SerializeNode(ezStreamWriter& stream) const = 0;
  virtual ezResult DeserializeNode(ezStreamReader& stream) = 0;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) = 0;
};

/// \brief Helper class for the common feature of fading in and out of an animation
///
/// This class will ramp up or down a weight value over time, such that an animation can be smoothly faded in or out
/// instead of just popping on and off.
struct EZ_RENDERERCORE_DLL ezAnimRampUpDown
{
  ezTime m_RampUp;   // [ property ]
  ezTime m_RampDown; // [ property ]

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, ezTime tDiff) const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimRampUpDown);
