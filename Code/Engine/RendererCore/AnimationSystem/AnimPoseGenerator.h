#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/RendererCoreDLL.h>

#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

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
  ezAnimPoseGeneratorCommand();
  ~ezAnimPoseGeneratorCommand();

  ezHybridArray<ezAnimPoseGeneratorCommandID, 4> m_Inputs;

  ezAnimPoseGeneratorCommandID GetCommandID() const { return m_CommandID; }
  ezAnimPoseGeneratorCommandType GetType() const { return m_Type; }

protected:
  void ConfigureCmd(ezAnimPoseGeneratorCommandType type, ezAnimPoseGeneratorCommandID cmdId);

private:
  friend class ezAnimPoseGenerator;

  bool m_bExecuted = false;
  ezAnimPoseGeneratorCommandID m_CommandID = ezInvalidIndex;
  ezAnimPoseGeneratorCommandType m_Type = ezAnimPoseGeneratorCommandType::Invalid;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandSampleTrack final : public ezAnimPoseGeneratorCommand
{
  ezAnimPoseGeneratorCommandSampleTrack();
  ~ezAnimPoseGeneratorCommandSampleTrack();

  void Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorLocalPoseID poseID);
  void Validate(const ezAnimPoseGenerator& queue) const;

  ezAnimationClipResourceHandle m_hAnimationClip;
  ezTime m_SampleTime;

protected:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandCombinePoses final : public ezAnimPoseGeneratorCommand
{
  ezAnimPoseGeneratorCommandCombinePoses();
  ~ezAnimPoseGeneratorCommandCombinePoses();

  void Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorLocalPoseID poseID);
  void Validate(const ezAnimPoseGenerator& queue) const;

  ezHybridArray<float, 4> m_InputWeights;

protected:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandLocalToModelPose final : public ezAnimPoseGeneratorCommand
{
  ezAnimPoseGeneratorCommandLocalToModelPose();
  ~ezAnimPoseGeneratorCommandLocalToModelPose();

  void Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorModelPoseID poseID);
  void Validate(const ezAnimPoseGenerator& queue) const;

  ezGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class ezAnimPoseGenerator;

  ezAnimPoseGeneratorModelPoseID m_ModelPoseOutput = ezInvalidIndex;
};

struct EZ_RENDERERCORE_DLL ezAnimPoseGeneratorCommandModelPoseToOutput final : public ezAnimPoseGeneratorCommand
{
  ezAnimPoseGeneratorCommandModelPoseToOutput();
  ~ezAnimPoseGeneratorCommandModelPoseToOutput();

  void Configure(ezAnimPoseGeneratorCommandID cmdId);
  void Validate(const ezAnimPoseGenerator& queue) const;
};

class EZ_RENDERERCORE_DLL ezAnimPoseGenerator final
{
public:
  void Reset(const ezSkeletonResource* pSkeleton);

  ezAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack();
  ezAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  ezAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  ezAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();

  const ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id) const;
  ezAnimPoseGeneratorCommand& GetCommand(ezAnimPoseGeneratorCommandID id);

  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>* GeneratePose();

private:
  void Validate() const;

  void Execute(ezAnimPoseGeneratorCommand& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(ezAnimPoseGeneratorCommandModelPoseToOutput& cmd);

  ozz::vector<ozz::math::SoaTransform>* AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id);
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>* AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id);

  const ezSkeletonResource* m_pSkeleton = nullptr;

  ezAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  ezAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>* m_pOutputPose = nullptr;

  ezDeque<ozz::vector<ozz::math::SoaTransform>> m_LocalTransforms;
  ezDynamicArray<ozz::vector<ozz::math::SoaTransform>*> m_UsedLocalTransforms;

  ezDeque<ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>> m_ModelTransforms;
  ezDynamicArray<ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>*> m_UsedModelTransforms;

  // TODO: use ezFrameAllocator::GetCurrentAllocator() ? (or pass in custom allocator?)
  ezDynamicArray<ezAnimPoseGeneratorCommand*> m_Commands;
  ezDeque<ezAnimPoseGeneratorCommandSampleTrack> m_CommandsSampleTrack;
  ezDeque<ezAnimPoseGeneratorCommandCombinePoses> m_CommandsCombinePoses;
  ezDeque<ezAnimPoseGeneratorCommandLocalToModelPose> m_CommandsLocalToModelPose;
  ezDeque<ezAnimPoseGeneratorCommandModelPoseToOutput> m_CommandsModelPoseToOutput;
};
