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

enum class ezAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  CombinePoses,
  LocalToModelPose,
  ModelPoseToOutput,
};

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

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandSampleTrack final : public ezAnimPoseGeneratorCommand
{
  ezAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;

private:
  friend class ezAnimPoseGenerator;

  ezUInt32 m_uiUniqueID = 0;
  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandCombinePoses final : public ezAnimPoseGeneratorCommand
{
  ezHybridArray<float, 4> m_InputWeights;
  ezHybridArray<ezArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandLocalToModelPose final : public ezAnimPoseGeneratorCommand
{
  ezGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorModelPoseID m_ModelPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandModelPoseToOutput final : public ezAnimPoseGeneratorCommand
{
};

class EZ_RENDERERCORE_DLL ezAnimPoseGenerator final
{
public:
  ezAnimPoseGenerator();
  ~ezAnimPoseGenerator();

  void Reset(const ezSkeletonResource* pSkeleton);

  ezAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(ezUInt32 uiDeterministicID);
  ezAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  ezAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  ezAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();

  const ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id) const;
  ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id);

  ezArrayPtr<ezMat4> GeneratePose();

private:
  void Validate() const;

  void Execute(ezAnimPoseGeneratorCommand& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandModelPoseToOutput& cmd);

  ezArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id);
  ezArrayPtr<ezMat4> AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id);

  const ezSkeletonResource* m_pSkeleton = nullptr;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  ezAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  ezArrayPtr<ezMat4> m_OutputPose;

  ezHybridArray<ezArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  ezHybridArray<ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  ezHybridArray<ezAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  ezHybridArray<ezAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  ezHybridArray<ezAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  ezHybridArray<ezAnimPoseGeneratorCommandModelPoseToOutput, 1> m_CommandsModelPoseToOutput;

  ezArrayMap<ezUInt32, ozz::animation::SamplingCache*> m_SamplingCaches;
};
