#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

class ezGameObject;
class ezAnimGraph;

using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;
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
  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  bool m_bUseRootMotion = false;
};

struct ezAnimGraphPinDataModelTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

class EZ_RENDERERCORE_DLL ezAnimController
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimController);

public:
  ezAnimController();
  ~ezAnimController();

  void Initialize(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard = nullptr);

  bool Update(ezTime diff, ezGameObject* pTarget, bool bEnableIK);

  void GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const;

  const ezSharedPtr<ezBlackboard>& GetBlackboard() { return m_pBlackboard; }

  ezAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);

  void AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms);

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void AddAnimGraph(const ezAnimGraphResourceHandle& hGraph);
  // TODO void RemoveAnimGraph(const ezAnimGraphResource& hGraph);

  struct AnimClipInfo
  {
    ezAnimationClipResourceHandle m_hClip;
  };

  const AnimClipInfo& GetAnimationClipInfo(ezTempHashedString sClipName) const;

private:
  void GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton);

  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;

  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_BlendMask;

  ezAnimPoseGenerator* m_pPoseGenerator = nullptr;
  ezSharedPtr<ezBlackboard> m_pBlackboard = nullptr;

  ezHybridArray<ezUInt32, 8> m_CurrentLocalTransformOutputs;

  static ezMutex s_SharedDataMutex;
  static ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> s_SharedBoneWeights;

  struct GraphInstance
  {
    ezAnimGraphResourceHandle m_hAnimGraph;
    ezUniquePtr<ezAnimGraphInstance> m_pInstance;
  };

  ezHybridArray<GraphInstance, 2> m_Instances;

  AnimClipInfo m_InvalidClipInfo;
  ezHashTable<ezHashedString, AnimClipInfo> m_AnimationClipMapping;

private:
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
  friend class ezAnimGraphBoolInputPin;
  friend class ezAnimGraphBoolOutputPin;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;
};
