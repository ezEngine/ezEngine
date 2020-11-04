#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezSkeletonResource;
class ezStreamWriter;
class ezStreamReader;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class EZ_RENDERERCORE_DLL ezPlayClipAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlayClipAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayClipAnimNode

public:
  void SetAnimationClip(const char* szFile); // [ property ]
  const char* GetAnimationClip() const;      // [ property ]

  void SetPartialBlendingRootBone(const char* szBone); // [ property ]
  const char* GetPartialBlendingRootBone() const;      // [ property ]

  ezTime m_RampUp;       // [ property ]
  ezTime m_RampDown;     // [ property ]
  float m_fSpeed = 1.0f; // [ property ]

  ezAnimationClipResourceHandle m_hAnimationClip;

private:
  ezAnimGraphTriggerInputPin m_Active; // [ property ]

  ezHashedString m_sPartialBlendingRootBone;
  ezTime m_PlaybackTime;
  float m_fCurWeight = 0.0f;
  bool m_bIsRampingUpOrDown = false;

  ezAnimGraphSamplingCache* m_pSamplingCache = nullptr;
  ezAnimGraphLocalTransforms* m_pLocalTransforms = nullptr;
  ezAnimGraphBlendWeights* m_pPartialBlendingMask = nullptr;
};
