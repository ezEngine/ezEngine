#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RendererCoreDLL.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_float.h>
#include <ozz/base/maths/soa_transform.h>

EZ_DEFINE_AS_POD_TYPE(ozz::math::SoaTransform);

class ezSkeletonResource;
class ezAnimPoseGenerator;
class ezGameObject;

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

using ezAnimPoseGeneratorLocalPoseID = ezUInt32;
using ezAnimPoseGeneratorModelPoseID = ezUInt32;
using ezAnimPoseGeneratorCommandID = ezUInt32;

/// \brief The type of ezAnimPoseGeneratorCommand
enum class ezAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  RestPose,
  CombinePoses,
  LocalToModelPose,
  ModelPoseToOutput,
  SampleEventTrack,
};

enum class ezAnimPoseEventTrackSampleMode : ezUInt8
{
  None,         ///< Don't sample the event track at all
  OnlyBetween,  ///< Sample the event track only between PrevSamplePos and SamplePos
  LoopAtEnd,    ///< Sample the event track between PrevSamplePos and End, then Start and SamplePos
  LoopAtStart,  ///< Sample the event track between PrevSamplePos and Start, then End and SamplePos
  BounceAtEnd,  ///< Sample the event track between PrevSamplePos and End, then End and SamplePos
  BounceAtStart ///< Sample the event track between PrevSamplePos and Start, then Start and SamplePos
};

/// \brief Base class for all pose generator commands
///
/// All commands have a unique command ID with which they are referenced.
/// All commands can have zero or N other commands set as *inputs*.
/// Every type of command only accepts certain types and amount of inputs.
///
/// The pose generation graph is built by allocating commands on the graph and then setting up
/// which command is an input to which other node.
/// A command can be an input to multiple other commands. It will be evaluated only once.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommand
{
  ezHybridArray<ezAnimPoseGeneratorCommandID, 4> m_Inputs;

  ezAnimPoseGeneratorCommandID GetCommandID() const { return m_CommandID; }
  ezAnimPoseGeneratorCommandType GetType() const { return m_Type; }

private:
  friend class ezAnimPoseGenerator;

  bool m_bExecuted = false;
  ezAnimPoseGeneratorCommandID m_CommandID = ezInvalidIndex;
  ezAnimPoseGeneratorCommandType m_Type = ezAnimPoseGeneratorCommandType::Invalid;
};

/// \brief Returns the rest pose (also often called 'bind pose').
///
/// The command has to be added as an input to one of
/// * ezAnimPoseGeneratorCommandCombinePoses
/// * ezAnimPoseGeneratorCommandLocalToModelPose
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandRestPose final : public ezAnimPoseGeneratorCommand
{
private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

/// \brief Samples an animation clip at a given time and optionally also its event track.
///
/// The command has to be added as an input to one of
/// * ezAnimPoseGeneratorCommandCombinePoses
/// * ezAnimPoseGeneratorCommandLocalToModelPose
///
/// If the event track shall be sampled as well, event messages are sent to the ezGameObject for which the pose is generated.
///
/// This command can optionally have input commands of type ezAnimPoseGeneratorCommandSampleEventTrack.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandSampleTrack final : public ezAnimPoseGeneratorCommand
{
  ezAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  ezAnimPoseEventTrackSampleMode m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

private:
  friend class ezAnimPoseGenerator;

  bool m_bAdditive = false;
  ezUInt32 m_uiUniqueID = 0;
  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

/// \brief Combines all the local space poses that are given as input into one local pose.
///
/// The input commands must be of type
/// * ezAnimPoseGeneratorCommandSampleTrack
/// * ezAnimPoseGeneratorCommandCombinePoses
/// * ezAnimPoseGeneratorCommandRestPose
///
/// Every input pose gets both an overall weight, as well as optionally a per-bone weight mask.
/// If a per-bone mask is used, the respective input pose will only affect those bones.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandCombinePoses final : public ezAnimPoseGeneratorCommand
{
  ezHybridArray<float, 4> m_InputWeights;
  ezHybridArray<ezArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * ezAnimPoseGeneratorCommandSampleTrack
/// * ezAnimPoseGeneratorCommandCombinePoses
/// * ezAnimPoseGeneratorCommandRestPose
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandLocalToModelPose final : public ezAnimPoseGeneratorCommand
{
  ezGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorModelPoseID m_ModelPoseOutput = ezInvalidIndex;
};

/// \brief Accepts a single input command that outputs a model space pose and forwards it to the ezGameObject for which the pose is generated.
///
/// The input command must be of type
/// * ezAnimPoseGeneratorCommandLocalToModelPose
///
/// Every graph should have exactly one of these nodes. Commands that are not (indirectly) connected to an
/// output node will not be evaluated and won't have any effect.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandModelPoseToOutput final : public ezAnimPoseGeneratorCommand
{
};

/// \brief Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * ezAnimPoseGeneratorCommandSampleTrack
/// * ezAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandSampleEventTrack final : public ezAnimPoseGeneratorCommand
{
  ezAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  ezAnimPoseEventTrackSampleMode m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

private:
  friend class ezAnimPoseGenerator;

  ezUInt32 m_uiUniqueID = 0;
};

class EZ_RENDERERCORE_DLL ezAnimPoseGenerator final
{
public:
  ezAnimPoseGenerator();
  ~ezAnimPoseGenerator();

  void Reset(const ezSkeletonResource* pSkeleton);

  ezAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(ezUInt32 uiDeterministicID);
  ezAnimPoseGeneratorCommandRestPose& AllocCommandRestPose();
  ezAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  ezAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  ezAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();
  ezAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();

  const ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id) const;
  ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id);

  ezArrayPtr<ezMat4> GeneratePose(const ezGameObject* pSendAnimationEventsTo);

private:
  void Validate() const;

  void Execute(ezAnimPoseGeneratorCommand& cmd, const ezGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd, const ezGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(ezAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandLocalToModelPose& cmd, const ezGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(ezAnimPoseGeneratorCommandModelPoseToOutput& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleEventTrack& cmd, const ezGameObject* pSendAnimationEventsTo);
  void SampleEventTrack(const ezAnimationClipResource* pResource, ezAnimPoseEventTrackSampleMode mode, const ezGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos);

  ezArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id);
  ezArrayPtr<ezMat4> AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id);

  const ezSkeletonResource* m_pSkeleton = nullptr;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  ezAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  ezArrayPtr<ezMat4> m_OutputPose;

  ezHybridArray<ezArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  ezHybridArray<ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  ezHybridArray<ezAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  ezHybridArray<ezAnimPoseGeneratorCommandRestPose, 1> m_CommandsRestPose;
  ezHybridArray<ezAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  ezHybridArray<ezAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  ezHybridArray<ezAnimPoseGeneratorCommandModelPoseToOutput, 1> m_CommandsModelPoseToOutput;
  ezHybridArray<ezAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;

  ezArrayMap<ezUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
