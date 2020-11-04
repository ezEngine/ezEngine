#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/animation/runtime/sampling_job.h>

class ezGameObject;
class ezStreamWriter;
class ezStreamReader;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

struct ezAnimGraphBlendWeights
{
  ozz::vector<ozz::math::SimdFloat4> m_ozzBlendWeights;
};

struct ezAnimGraphLocalTransforms
{
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
};

struct ezAnimGraphSamplingCache
{
  ozz::animation::SamplingCache m_ozzSamplingCache;
};

class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  void Update(ezTime tDiff);
  void SendResultTo(ezGameObject* pObject);
  const ezVec3& GetRootMotion() const { return m_vRootMotion; }

  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;

  ezSkeletonResourceHandle m_hSkeleton;

  ezBlackboard m_Blackboard;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezDynamicArray<ezInt8> m_TriggerInputPinStates;
  ezDynamicArray<ezDynamicArray<ezUInt16>> m_TriggerOutputToInputPinMapping;

  /// \brief To be called by ezAnimGraphNode classes every frame that they want to affect animation
  void AddFrameBlendLayer(const ozz::animation::BlendingJob::Layer& layer);

  /// \brief To be called by ezAnimGraphNode classes every frame that they want to affect the root motion
  void AddFrameRootMotion(const ezVec3& motion);

  ezAnimGraphBlendWeights* AllocateBlendWeights(const ezSkeletonResource& skeleton);
  void FreeBlendWeights(ezAnimGraphBlendWeights*& pWeights);

  ezAnimGraphLocalTransforms* AllocateLocalTransforms(const ezSkeletonResource& skeleton);
  void FreeLocalTransforms(ezAnimGraphLocalTransforms*& pTransforms);

  ezAnimGraphSamplingCache* AllocateSamplingCache(const ozz::animation::Animation& animclip);
  void FreeSamplingCache(ezAnimGraphSamplingCache*& pTransforms);

private:
  ezDynamicArray<ozz::animation::BlendingJob::Layer> m_ozzBlendLayers;
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_ModelSpaceTransforms;

  ezDeque<ezAnimGraphBlendWeights> m_BlendWeights;
  ezHybridArray<ezAnimGraphBlendWeights*, 8> m_BlendWeightsFreeList;

  ezDeque<ezAnimGraphLocalTransforms> m_LocalTransforms;
  ezHybridArray<ezAnimGraphLocalTransforms*, 8> m_LocalTransformsFreeList;

  ezDeque<ezAnimGraphSamplingCache> m_SamplingCaches;
  ezHybridArray<ezAnimGraphSamplingCache*, 8> m_SamplingCachesFreeList;

  bool m_bFinalized = false;
  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  void Finalize(const ezSkeletonResource* pSkeleton);
};
