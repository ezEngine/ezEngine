#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// Outputs the skeleton's rest/bind pose.
///
/// This node provides the default T-pose or A-pose from the skeleton definition. Commonly used as a fallback
/// when no animation is active, as a base for additive blending, or when animations fail to load.
class EZ_RENDERERCORE_DLL ezRestPoseAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRestPoseAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezRestPoseAnimNode

private:
  ezAnimGraphLocalPoseOutputPin m_OutPose; // [ property ]
};
