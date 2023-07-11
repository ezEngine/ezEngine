#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class ezSkeletonResource;
class ezGameObject;
class ezAnimGraphInstance;
class ezAnimController;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphPinDataLocalTransforms;
struct ezAnimGraphPinDataBoneWeights;
class ezAnimationClipResource;
struct ezInstanceDataDesc;

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an ezAnimGraphInstance
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

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class ezAnimGraphInstance;
  friend class ezAnimGraph;
  friend class ezAnimGraphResource;

  ezHashedString m_sCustomNodeTitle;
  ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;

  virtual ezResult SerializeNode(ezStreamWriter& stream) const = 0;
  virtual ezResult DeserializeNode(ezStreamReader& stream) = 0;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const = 0;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezAnimState
{
  enum class State
  {
    Off,
    StartedRampUp,
    RampingUp,
    Running,
    StartedRampDown,
    RampingDown,
    Finished,
  };

  // Properties:
  ezTime m_FadeIn;                  // [ property ]
  ezTime m_FadeOut;                 // [ property ]
  bool m_bImmediateFadeIn = false;  // [ property ]
  bool m_bImmediateFadeOut = false; // [ property ]
  bool m_bLoop = false;             // [ property ]
  float m_fPlaybackSpeed = 1.0f;    // [ property ]
  bool m_bApplyRootMotion = false;  // [ property ]

  // Inputs:
  bool m_bTriggerActive = false;
  float m_fPlaybackSpeedFactor = 1.0f;
  ezTime m_Duration;
  ezTime m_DurationOfQueued;

  bool WillStateBeOff(bool bTriggerActive) const;
  void UpdateState(ezTime diff);
  State GetCurrentState() const { return m_State; }
  float GetWeight() const { return m_fCurWeight; }
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }
  bool HasTransitioned() const { return m_bHasTransitioned; }
  bool HasLoopedStart() const { return m_bHasLoopedStart; }
  bool HasLoopedEnd() const { return m_bHasLoopedEnd; }
  float GetFinalSpeed() const { return m_fPlaybackSpeed * m_fPlaybackSpeedFactor; }

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

private:
  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, ezTime tDiff) const;

  State m_State = State::Off;
  float m_fNormalizedPlaybackPosition = 0.0f;
  bool m_bRequireLoopForRampDown = true;
  bool m_bHasTransitioned = false;
  bool m_bHasLoopedStart = false;
  bool m_bHasLoopedEnd = false;
  float m_fCurWeight = 0.0f;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimState);
