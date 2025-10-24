#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Linearly interpolates (blends) between multiple poses.
///
/// This node blends 2 or more poses using a lerp parameter (0-1 or 0-N for multiple poses).
/// Weights are automatically normalized. Common use cases include blending walk and run animations,
/// or smoothly transitioning between any set of poses.
class EZ_RENDERERCORE_DLL ezLerpPosesAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLerpPosesAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLerpPosesAnimNode

public:
  ezLerpPosesAnimNode();
  ~ezLerpPosesAnimNode();

  float m_fLerp = 0.5f;                                     // [ property ]

private:
  ezUInt8 m_uiPosesCount = 0;                               // [ property ]
  ezHybridArray<ezAnimGraphLocalPoseInputPin, 2> m_InPoses; // [ property ]
  ezAnimGraphNumberInputPin m_InLerp;                       // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;                  // [ property ]
};
