#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class EZ_RENDERERCORE_DLL ezPlayClipOnceAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlayClipOnceAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayClipOnceAnimNode

public:
  ezUInt32 Clips_GetCount() const;                          // [ property ]
  const char* Clips_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Clips_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
  void Clips_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
  void Clips_Remove(ezUInt32 uiIndex);                      // [ property ]

private:
  ezHybridArray<ezAnimationClipResourceHandle, 1> m_Clips; // [ property ]

  ezAnimGraphTriggerInputPin m_ActivatePin;     // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphNumberInputPin m_ClipIndexPin;     // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFinishedPin;  // [ property ]

  float m_fPlaybackSpeed = 1.0f; // [ property ]

  ezUInt8 m_uiClipToPlay = 0xFF;
  ezTime m_PlaybackTime;
};
