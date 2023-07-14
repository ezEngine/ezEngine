#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class ezGameObject;
class ezAnimGraph;
class ezAnimController;

class EZ_RENDERERCORE_DLL ezAnimGraphInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraphInstance);

public:
  ezAnimGraphInstance();
  ~ezAnimGraphInstance();

  void Configure(const ezAnimGraph& animGraph);

  void Update(ezAnimController& ref_controller, ezTime diff, ezGameObject* pTarget, const ezSkeletonResource* pSekeltonResource);

  template <typename T>
  T* GetAnimNodeInstanceData(const ezAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }


private:
  const ezAnimGraph* m_pAnimGraph = nullptr;

  ezBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  ezInt8* m_pTriggerInputPinStates = nullptr;
  double* m_pNumberInputPinStates = nullptr;
  bool* m_pBoolInputPinStates = nullptr;
  ezUInt16* m_pBoneWeightInputPinStates = nullptr;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_LocalPoseInputPinStates;
  ezUInt16* m_pModelPoseInputPinStates = nullptr;

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
};
