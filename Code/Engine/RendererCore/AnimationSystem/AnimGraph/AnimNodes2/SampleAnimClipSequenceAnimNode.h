#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class EZ_RENDERERCORE_DLL ezSampleAnimClipSequenceAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleAnimClipSequenceAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSampleAnimClipSequenceAnimNode

public:
  ezSampleAnimClipSequenceAnimNode();
  ~ezSampleAnimClipSequenceAnimNode();

  void SetStartClip(const char* szClip);
  const char* GetStartClip() const;

  ezUInt32 Clips_GetCount() const;                            // [ property ]
  const char* Clips_GetValue(ezUInt32 uiIndex) const;         // [ property ]
  void Clips_SetValue(ezUInt32 uiIndex, const char* szValue); // [ property ]
  void Clips_Insert(ezUInt32 uiIndex, const char* szValue);   // [ property ]
  void Clips_Remove(ezUInt32 uiIndex);                        // [ property ]

  void SetEndClip(const char* szClip);
  const char* GetEndClip() const;

private:
  ezHashedString m_sStartClip;                      // [ property ]
  ezHybridArray<ezHashedString, 1> m_Clips;         // [ property ]
  ezHashedString m_sEndClip;                        // [ property ]
  bool m_bApplyRootMotion = false;                  // [ property ]
  bool m_bLoop = false;                             // [ property ]
  float m_fPlaybackSpeed = 1.0f;                    // [ property ]

  ezAnimGraphTriggerInputPin m_InStart;             // [ property ]
  ezAnimGraphBoolInputPin m_InLoop;                 // [ property ]
  ezAnimGraphNumberInputPin m_InSpeed;              // [ property ]
  ezAnimGraphNumberInputPin m_ClipIndexPin;         // [ property ]

  ezAnimGraphLocalPoseOutputPin m_OutPose;          // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnMiddleStarted; // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnEndStarted;    // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFinished;      // [ property ]

  struct InstanceState
  {
    ezTime m_PlaybackTime;
    ezUInt8 m_uiState = 0; // 0 = off, 1 = start, 2 = middle, 3 = end
    ezUInt8 m_uiMiddleClipIdx = 0;
  };
};
