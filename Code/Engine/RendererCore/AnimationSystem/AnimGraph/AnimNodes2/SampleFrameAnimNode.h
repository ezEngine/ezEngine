#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class EZ_RENDERERCORE_DLL ezSampleFrameAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleFrameAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSampleFrameAnimNode

public:
  void SetClip(const char* szClip);
  const char* GetClip() const;

  ezAnimationClipResourceHandle m_hClip;    // [ property ]
  float m_fNormalizedSamplePosition = 0.0f; // [ property ]

private:
  ezAnimGraphNumberInputPin m_InNormalizedSamplePosition; // [ property ]
  ezAnimGraphNumberInputPin m_InAbsoluteSamplePosition;   // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;                // [ property ]
};