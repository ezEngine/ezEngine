#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

class ezGameObject;
class ezAnimGraph;

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

class EZ_RENDERERCORE_DLL ezAnimGraphInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraphInstance);

public:
  ezAnimGraphInstance();
  ~ezAnimGraphInstance();

  void Configure(const ezAnimGraph& animGraph, const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard = nullptr);

  void Update(ezTime diff, ezGameObject* pTarget);
  void GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const;

  const ezSharedPtr<ezBlackboard>& GetBlackboard() { return m_pBlackboard; }

  ezAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);

  void AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms);

  template <typename T>
  T* GetAnimNodeInstanceData(const ezAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }

protected:
  void GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton);

private:
  const ezAnimGraph* m_pAnimGraph = nullptr;

  ezSkeletonResourceHandle m_hSkeleton;

  ezBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  ezInt8* m_pTriggerInputPinStates = nullptr;
  double* m_pNumberInputPinStates = nullptr;
  bool* m_pBoolInputPinStates = nullptr;
  ezUInt16* m_pBoneWeightInputPinStates = nullptr;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_LocalPoseInputPinStates;
  ezUInt16* m_pModelPoseInputPinStates = nullptr;

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
  friend class ezAnimGraphBoolInputPin;
  friend class ezAnimGraphBoolOutputPin;

  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_BlendMask;

  ezAnimPoseGenerator* m_pPoseGenerator = nullptr;
  ezSharedPtr<ezBlackboard> m_pBlackboard = nullptr;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  ezHybridArray<ezUInt32, 8> m_CurrentLocalTransformOutputs;

  static ezMutex s_SharedDataMutex;
  static ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> s_SharedBoneWeights;
};


class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  ezAnimGraphNode* AddNode(ezUniquePtr<ezAnimGraphNode>&& pNode);
  void AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, const ezAnimGraphNode* pDstNode, ezStringView sDstPinName);

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  const ezInstanceDataAllocator& GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  ezArrayPtr<const ezUniquePtr<ezAnimGraphNode>> GetNodes() const { return m_Nodes; }

  void PrepareForUse();

private:
  friend class ezAnimGraphInstance;

  struct ConnectionTo
  {
    ezString m_sSrcPinName;
    const ezAnimGraphNode* m_pDstNode = nullptr;
    ezString m_sDstPinName;
    ezAnimGraphPin* m_pSrcPin = nullptr;
    ezAnimGraphPin* m_pDstPin = nullptr;
  };

  struct ConnectionsTo
  {
    ezHybridArray<ConnectionTo, 2> m_To;
  };

  void SortNodesByPriority();
  void PreparePinMapping();
  void AssignInputPinIndices();
  void AssignOutputPinIndices();
  ezUInt16 ComputeNodePriority(const ezAnimGraphNode* pNode, ezMap<const ezAnimGraphNode*, ezUInt16>& inout_Prios, ezUInt16& inout_uiOutputPrio) const;

  bool m_bPreparedForUse = true;
  ezUInt32 m_uiInputPinCounts[ezAnimGraphPin::Type::ENUM_COUNT];
  ezUInt32 m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::ENUM_COUNT];
  ezMap<const ezAnimGraphNode*, ConnectionsTo> m_From;

  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_OutputPinToInputPinMapping[ezAnimGraphPin::ENUM_COUNT];
  ezInstanceDataAllocator m_InstanceDataAllocator;

  friend class ezAnimGraphTriggerOutputPin;
  friend class ezAnimGraphNumberOutputPin;
  friend class ezAnimGraphBoolOutputPin;
  friend class ezAnimGraphBoneWeightsOutputPin;
  friend class ezAnimGraphLocalPoseOutputPin;
  friend class ezAnimGraphModelPoseOutputPin;
};
