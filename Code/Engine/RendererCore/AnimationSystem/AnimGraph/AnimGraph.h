#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezGameObject;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

struct ezAnimGraphBoneWeights
{
  float m_fOverallWeight = 1.0f;
  ozz::vector<ozz::math::SimdFloat4> m_ozzBoneWeights;
};

struct ezAnimGraphLocalTransforms
{
  const ezAnimGraphBoneWeights* m_pWeights = nullptr;
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
  float m_fOverallWeight = 1.0f;
  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  bool m_bUseRootMotion = false;
};

struct ezAnimGraphSamplingCache
{
  const ozz::animation::Animation* m_pUsedForAnim = nullptr;
  ozz::animation::SamplingCache m_ozzSamplingCache;
};

struct ezAnimGraphModelTransforms
{
  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  bool m_bUseRootMotion = false;
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_ModelTransforms;
};

class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  void Update(ezTime tDiff, ezGameObject* pTarget);
  ezVec3 GetRootMotion() const;

  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;

  ezSkeletonResourceHandle m_hSkeleton;

  void SetExternalBlackboard(ezBlackboard* pBlackboard);
  ezBlackboard& GetBlackboard()
  {
    return *m_pBlackboard;
  }

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezDynamicArray<ezDynamicArray<ezUInt16>> m_OutputPinToInputPinMapping[ezAnimGraphPin::ENUM_COUNT];

  // EXTEND THIS if a new type is introduced
  ezDynamicArray<ezInt8> m_TriggerInputPinStates;
  ezDynamicArray<double> m_NumberInputPinStates;
  ezDynamicArray<ezAnimGraphBoneWeights*> m_BoneWeightInputPinStates;
  ezDynamicArray<ezHybridArray<ezAnimGraphLocalTransforms*, 1>> m_LocalPoseInputPinStates;
  ezDynamicArray<ezAnimGraphModelTransforms*> m_ModelPoseInputPinStates;

  ezAnimGraphBoneWeights* AllocateBoneWeights(const ezSkeletonResource& skeleton);
  void FreeBoneWeights(ezAnimGraphBoneWeights*& pWeights);

  ezAnimGraphLocalTransforms* AllocateLocalTransforms(const ezSkeletonResource& skeleton);
  void FreeLocalTransforms(ezAnimGraphLocalTransforms*& pTransforms);

  ezAnimGraphModelTransforms* AllocateModelTransforms(const ezSkeletonResource& skeleton);
  void FreeModelTransforms(ezAnimGraphModelTransforms*& pTransforms);

  ezAnimGraphSamplingCache* AllocateSamplingCache(const ozz::animation::Animation& animclip);
  void UpdateSamplingCache(ezAnimGraphSamplingCache*& pCache, const ozz::animation::Animation& animclip);
  void FreeSamplingCache(ezAnimGraphSamplingCache*& pTransforms);

  ezAnimGraphModelTransforms* m_pCurrentModelTransforms = nullptr;

private:
  ezBlackboard* m_pBlackboard = nullptr;
  ezBlackboard m_Blackboard;

  ezDeque<ezAnimGraphBoneWeights> m_BoneWeights;
  ezHybridArray<ezAnimGraphBoneWeights*, 8> m_BoneWeightsFreeList;

  ezDeque<ezAnimGraphLocalTransforms> m_LocalTransforms;
  ezHybridArray<ezAnimGraphLocalTransforms*, 8> m_LocalTransformsFreeList;

  ezDeque<ezAnimGraphModelTransforms> m_ModelTransforms;
  ezHybridArray<ezAnimGraphModelTransforms*, 8> m_ModelTransformsFreeList;

  ezDeque<ezAnimGraphSamplingCache> m_SamplingCaches;
  ezHybridArray<ezAnimGraphSamplingCache*, 8> m_SamplingCachesFreeList;
};
