#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class EZ_RENDERERCORE_DLL ezSampleAnimClipAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleAnimClipAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSampleAnimClipAnimNode

  void SetClip(const char* szClip);
  const char* GetClip() const;

public:
  ezSampleAnimClipAnimNode();
  ~ezSampleAnimClipAnimNode();

private:
  ezHashedString m_sClip;                      // [ property ]
  bool m_bLoop = true;                         // [ property ]
  float m_fRootMotionAmount = 0.0f;            // [ property ]
  float m_fPlaybackSpeed = 1.0f;               // [ property ]

  ezAnimGraphTriggerInputPin m_InStart;        // [ property ]
  ezAnimGraphBoolInputPin m_InLoop;            // [ property ]
  ezAnimGraphNumberInputPin m_InSpeed;         // [ property ]

  ezAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]

  struct InstanceState
  {
    ezTime m_PlaybackTime = ezTime::MakeFromHours(1000);
  };
};
