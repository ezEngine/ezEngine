#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezPlaySequenceAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlaySequenceAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlaySequenceAnimNode

public:
  void SetStartClip(const char* szFile); // [ property ]
  const char* GetStartClip() const;      // [ property ]

  ezUInt32 MiddleClips_GetCount() const;                          // [ property ]
  const char* MiddleClips_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void MiddleClips_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
  void MiddleClips_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
  void MiddleClips_Remove(ezUInt32 uiIndex);                      // [ property ]

  void SetEndClip(const char* szFile); // [ property ]
  const char* GetEndClip() const;      // [ property ]

  ezAnimationClipResourceHandle m_hStartClip;
  ezHybridArray<ezAnimationClipResourceHandle, 2> m_hMiddleClips;
  ezAnimationClipResourceHandle m_hEndClip;

private:
  ezAnimGraphTriggerInputPin m_ActivePin;           // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;      // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;             // [ property ]
  ezAnimGraphNumberInputPin m_ClipIndexPin;         // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin;     // [ property ]
  ezAnimGraphTriggerOutputPin m_OnNextClipPin;      // [ property ]
  ezAnimGraphNumberOutputPin m_PlayingClipIndexPin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFadeOutPin;       // [ property ]

  ezAnimState m_State;

  enum class Phase : ezUInt8
  {
    Off,
    Start,
    Middle,
    End,
  };

  Phase m_Phase = Phase::Off;
  ezUInt8 m_uiClipToPlay = 0xFF;
  ezUInt8 m_uiNextClipToPlay = 0xFF;
};
