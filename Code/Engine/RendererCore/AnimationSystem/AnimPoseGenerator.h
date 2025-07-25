#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>
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
  SampleEventTrack,
  AimIK,
  TwoBoneIK,
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

/// \brief Base class for commands that produce or update a model pose.
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandModelPose : public ezAnimPoseGeneratorCommand
{
protected:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorModelPoseID m_ModelPoseOutput = ezInvalidIndex;
  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * ezAnimPoseGeneratorCommandSampleTrack
/// * ezAnimPoseGeneratorCommandCombinePoses
/// * ezAnimPoseGeneratorCommandRestPose
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandLocalToModelPose final : public ezAnimPoseGeneratorCommandModelPose
{
  ezGameObject* m_pSendLocalPoseMsgTo = nullptr;
};

/// \brief Accepts a single input in model space and applies aim IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * ezAnimPoseGeneratorCommandLocalToModelPose
/// * ezAnimPoseGeneratorCommandAimIK
/// * ezAnimPoseGeneratorCommandTwoBoneIK
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandAimIK final : public ezAnimPoseGeneratorCommandModelPose
{
  float m_fDebugVisScale = 0.0f;                                ///< If >0, debug visualization will be rendered.
  ezVec3 m_vTargetPosition;                                     ///< The position for the bone to point at. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  ezUInt16 m_uiJointIdx;                                        ///< The index of the joint to aim.
  ezUInt16 m_uiRecalcModelPoseToJointIdx = ezInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  float m_fWeight = 1.0f;                                       ///< Factor between 0 and 1 for how much to apply the IK.
  ezVec3 m_vForwardVector = ezVec3::MakeAxisX();                ///< The local joint direction that should aim at the target. Typically there is a convention to use +X, +Y or +Z.
  ezVec3 m_vUpVector = ezVec3::MakeAxisZ();                     ///< The local joint direction that should point towards the pole vector. Must be orthogonal to the forward vector.
  ezVec3 m_vPoleVectorPosition = ezVec3::MakeAxisY();           ///< In the same space as the target position, a position that the up vector of the joint should (roughly) point towards. Used to have bones point into the right direction, for example to make an elbow point properly sideways.
  bool m_bInversePoleVector = false;                            ///< If true, the pole vector direction will point away from the pole vector position, rather than towards it. Useful in an aim IK chain to have all bones point away from a central point.
};

/// \brief Accepts a single input in model space and applies two-bone IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * ezAnimPoseGeneratorCommandLocalToModelPose
/// * ezAnimPoseGeneratorCommandAimIK
/// * ezAnimPoseGeneratorCommandTwoBoneIK
struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandTwoBoneIK final : public ezAnimPoseGeneratorCommandModelPose
{
  float m_fDebugVisScale = 0.0f;                                ///< If >0, debug visualization will be rendered.
  ezVec3 m_vTargetPosition;                                     ///< The position for the 'end' joint to try to reach. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  ezUInt16 m_uiJointIdxStart;                                   ///< Index of the top joint in a chain of three joints. The IK result may freely rotate around this joint into any (unnatural direction).
  ezUInt16 m_uiJointIdxMiddle;                                  ///< Index of the middle joint in a chain of three joints. The IK result will bend at this joint around the joint local mid-axis.
  ezUInt16 m_uiJointIdxEnd;                                     ///< Index of the end joint that is supposed to reach the target.
  ezUInt16 m_uiRecalcModelPoseToJointIdx = ezInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  ezVec3 m_vMidAxis = ezVec3::MakeAxisZ();                      ///< The local joint direction around which to bend the middle joint. Typically there is a convention to use +X, +Y or +Z to bend around.
  ezVec3 m_vPoleVectorPosition = ezVec3::MakeAxisY();           ///< In the same space as the target position, a position that the middle joint should (roughly) point towards. Used to have bones point into the right direction, for example to make a knee point properly forwards.
  float m_fWeight = 1.0f;                                       ///< Factor between 0 and 1 for how much to apply the IK.
  float m_fSoften = 1.0f;                                       ///< Factor between 0 and 1. See OZZ for details.
  ezAngle m_TwistAngle;                                         ///< After IK how much to rotate the chain. Seems to be redundant with the pole vector. See OZZ for details.
};

/// \brief Low-level infrastructure to generate animation poses from animation clips and other inputs.
///
/// Even though instances of this class should be reused over frames, it is assumed that all commands are recreated every frame, to build a new pose.
/// Some commands take predecessor commands as inputs to combine. If a command turns out not be actually needed, it won't be evaluated.
class EZ_RENDERERCORE_DLL ezAnimPoseGenerator final
{
public:
  ezAnimPoseGenerator();
  ~ezAnimPoseGenerator();

  void Reset(const ezSkeletonResource* pSkeleton, ezGameObject* pTarget);

  const ezSkeletonResource* GetSkeleton() const { return m_pSkeleton; }
  const ezGameObject* GetTargetObject() const { return m_pTargetGameObject; }

  ezAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(ezUInt32 uiDeterministicID);
  ezAnimPoseGeneratorCommandRestPose& AllocCommandRestPose();
  ezAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  ezAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  ezAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();
  ezAnimPoseGeneratorCommandAimIK& AllocCommandAimIK();
  ezAnimPoseGeneratorCommandTwoBoneIK& AllocCommandTwoBoneIK();

  const ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id) const;
  ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id);

  /// \brief Calculates the pose, using the final command as reference where to start.
  ///
  /// If bRequestExternalPoseGeneration is true, inverse-kinematics (IK) and powered ragdolls are also used.
  void UpdatePose(bool bRequestExternalPoseGeneration);

  ezArrayPtr<ezMat4> GetCurrentPose() const { return m_OutputPose; }

  /// \brief Sets the (currently) final command in the pose generation.
  ///
  /// This will be used to determine which other commands are necessary to calculate.
  void SetFinalCommand(ezAnimPoseGeneratorCommandID cmdId) { m_FinalCommand = cmdId; }

  /// \brief Returns the (currently) final command.
  ///
  /// Can be used to inject further commands after it. Call SetFinalCommand() afterwards.
  ezAnimPoseGeneratorCommandID GetFinalCommand() const { return m_FinalCommand; }

  /// \brief Whether the caller should send ezMsgAnimationPoseUpdated to its children.
  ///
  /// Typically this should be done, to forward the calculated pose to the animated mesh components.
  /// However, when some other component takes over control (usually a ragdoll),
  /// the pose from this generator isn't the final result and thus should not be forwarded.
  /// The other component will take care of this already.
  bool ShouldSendPoseResultMsg() const { return m_bSendResultMsg; }

private:
  void Validate() const;

  void Execute(ezAnimPoseGeneratorCommand& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleEventTrack& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandAimIK& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandTwoBoneIK& cmd);
  void SampleEventTrack(const ezAnimationClipResource* pResource, ezAnimPoseEventTrackSampleMode mode, float fPrevPos, float fCurPos);

  ezArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id);
  ezArrayPtr<ezMat4> AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id);

  ezGameObject* m_pTargetGameObject = nullptr;
  const ezSkeletonResource* m_pSkeleton = nullptr;

  ezArrayPtr<ezMat4> m_OutputPose;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  ezAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  bool m_bSendResultMsg = true;
  ezAnimPoseGeneratorCommandID m_FinalCommand = 0;

  ezHybridArray<ezArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  ezHybridArray<ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  ezHybridArray<ezAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  ezHybridArray<ezAnimPoseGeneratorCommandRestPose, 1> m_CommandsRestPose;
  ezHybridArray<ezAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  ezHybridArray<ezAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  ezHybridArray<ezAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;
  ezHybridArray<ezAnimPoseGeneratorCommandAimIK, 2> m_CommandsAimIK;
  ezHybridArray<ezAnimPoseGeneratorCommandTwoBoneIK, 2> m_CommandsTwoBoneIK;

  ezArrayMap<ezUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
