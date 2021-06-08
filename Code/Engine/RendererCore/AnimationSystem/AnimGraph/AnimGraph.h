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
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  void Configure(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& poseGenerator, ezBlackboard* pBlackboard = nullptr);

  void Update(ezTime tDiff, ezGameObject* pTarget);
  void GetRootMotion(ezVec3& translation, ezAngle& rotationX, ezAngle& rotationY, ezAngle& rotationZ) const;

  ezBlackboard* GetBlackboard() { return m_pBlackboard; }

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const ezVec3& translation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);

private:
  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;
  ezSkeletonResourceHandle m_hSkeleton;

  ezDynamicArray<ezDynamicArray<ezUInt16>> m_OutputPinToInputPinMapping[ezAnimGraphPin::ENUM_COUNT];

  // EXTEND THIS if a new type is introduced
  ezDynamicArray<ezInt8> m_TriggerInputPinStates;
  ezDynamicArray<double> m_NumberInputPinStates;
  ezDynamicArray<ezUInt16> m_BoneWeightInputPinStates;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_LocalPoseInputPinStates;
  ezDynamicArray<ezUInt16> m_ModelPoseInputPinStates;

  ezAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  ezVec3 m_vRootMotion = ezVec3::ZeroVector();
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;

private:
  friend class ezAnimationControllerAssetDocument;
  friend class ezAnimGraphTriggerOutputPin;
  friend class ezAnimGraphTriggerInputPin;
  friend class ezAnimGraphBoneWeightsInputPin;
  friend class ezAnimGraphBoneWeightsOutputPin;
  friend class ezAnimGraphLocalPoseInputPin;
  friend class ezAnimGraphLocalPoseOutputPin;
  friend class ezAnimGraphModelPoseInputPin;
  friend class ezAnimGraphModelPoseOutputPin;
  friend class ezAnimGraphLocalPoseMultiInputPin;
  friend class ezAnimGraphNumberInputPin;
  friend class ezAnimGraphNumberOutputPin;

  bool m_bInitialized = false;

  ezAnimPoseGenerator* m_pPoseGenerator = nullptr;
  ezBlackboard* m_pBlackboard = nullptr;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  static ezMutex s_SharedDataMutex;
  static ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> s_SharedBoneWeights;
};
