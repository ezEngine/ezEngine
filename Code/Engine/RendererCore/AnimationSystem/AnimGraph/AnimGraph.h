#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <ozz/animation/runtime/sampling_job.h>

class ezGameObject;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

EZ_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

struct ezAnimGraphPinDataBoneWeights
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const ezAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

struct ezAnimGraphPinDataLocalTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  const ezAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  bool m_bUseRootMotion = false;
};

struct ezAnimGraphPinDataModelTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  bool m_bUseRootMotion = false;
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
  ezDynamicArray<ezUInt16> m_BoneWeightInputPinStates;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_LocalPoseInputPinStates;
  ezDynamicArray<ezUInt16> m_ModelPoseInputPinStates;

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  ezAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  ezAnimPoseGenerator& GetPoseGenerator() { return m_PoseGenerator; }

  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

private:
  friend class ezAnimGraphBoneWeightsInputPin;
  friend class ezAnimGraphBoneWeightsOutputPin;
  friend class ezAnimGraphLocalPoseInputPin;
  friend class ezAnimGraphLocalPoseOutputPin;
  friend class ezAnimGraphModelPoseInputPin;
  friend class ezAnimGraphModelPoseOutputPin;
  friend class ezAnimGraphLocalPoseMultiInputPin;

  ezBlackboard* m_pBlackboard = nullptr;
  ezBlackboard m_Blackboard;

  ezAnimPoseGenerator m_PoseGenerator;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  static ezMutex s_SharedDataMutex;
  static ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> s_SharedBoneWeights;
};
