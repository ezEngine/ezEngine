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

class EZ_RENDERERCORE_DLL ezPlaySequenceAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlaySequenceAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayClipAnimNode

public:
  void SetStartClip(const char* szFile); // [ property ]
  const char* GetStartClip() const;      // [ property ]

  void SetMiddleClip(const char* szFile); // [ property ]
  const char* GetMiddleClip() const;      // [ property ]

  void SetEndClip(const char* szFile); // [ property ]
  const char* GetEndClip() const;      // [ property ]

  float m_fSpeed = 1.0f; // [ property ]

  ezAnimationClipResourceHandle m_hStartClip;
  ezAnimationClipResourceHandle m_hMiddleClip;
  ezAnimationClipResourceHandle m_hEndClip;

private:
  enum class State : ezUInt8
  {
    Off,
    Start,
    Loop,
    End
  };

  ezAnimGraphTriggerInputPin m_Active; // [ property ]

  State m_State = State::Off;
  ezTime m_PlaybackTime;

  ezAnimGraphSamplingCache* m_pSamplingCache = nullptr;
  ezAnimGraphLocalTransforms* m_pLocalTransforms = nullptr;
};
