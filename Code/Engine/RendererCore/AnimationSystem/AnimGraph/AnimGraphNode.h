#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class ezSkeletonResource;
class ezGameObject;
class ezAnimGraph;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphPinDataLocalTransforms;
struct ezAnimGraphPinDataBoneWeights;
class ezAnimationClipResource;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an ezAnimGraph
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

  const char* GetCustomNodeTitle() const { return m_CustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* sz) { m_CustomNodeTitle.Assign(sz); }

protected:
  friend class ezAnimGraph;

  ezHashedString m_CustomNodeTitle;

  virtual ezResult SerializeNode(ezStreamWriter& stream) const = 0;
  virtual ezResult DeserializeNode(ezStreamReader& stream) = 0;

  virtual void Initialize(ezAnimGraph& graph, const ezSkeletonResource* pSkeleton) {}
  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct ezComparisonOperator
{
  using StorageType = ezUInt8;
  enum Enum
  {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal
  };

  static bool Compare(ezComparisonOperator::Enum cmp, double f1, double f2);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezComparisonOperator);

/// \brief Helper class for the common feature of fading in and out of an animation
///
/// This class will ramp up or down a weight value over time, such that an animation can be smoothly faded in or out
/// instead of just popping on and off.
struct EZ_RENDERERCORE_DLL ezAnimRampUpDown
{
  ezTime m_RampUp;   // [ property ]
  ezTime m_RampDown; // [ property ]

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, ezTime tDiff) const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimRampUpDown);

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

  bool WillStateBeOff(bool bTriggerActive) const;
  void UpdateState(ezTime tDiff);

  ezAnimRampUpDown m_AnimRamp;
  bool m_bImmediateRampUp = true;
  bool m_bImmediateRampDown = true;
  bool m_bLoop = true;
  bool m_bTriggerActive = false;
  float m_fPlaybackSpeed = 1.0f;
  ezTime m_Duration;
  ezTime m_DurationOfQueued;

  State GetCurrentState() const { return m_State; }
  float GetWeight() const { return m_fCurWeight; }
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }
  bool HasTransitioned() const { return m_bHasTransitioned; }

private:
  State m_State = State::Off;
  float m_fNormalizedPlaybackPosition = 0.0f;
  bool m_bRequireLoopForRampDown = true;
  bool m_bHasTransitioned = false;
  float m_fCurWeight = 0.0f;
};
