#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class ezGameObject;
class ezAnimGraph;
class ezAnimController;

/// Runtime state for a single animation graph, owned by ezAnimController.
///
/// While ezAnimGraph defines the structure (nodes, connections, logic), ezAnimGraphInstance holds
/// the per-character runtime state needed to execute that graph. This allows one graph definition
/// to be shared by many characters, each with their own instance storing playback positions,
/// blend weights, transition states, and pin values.
///
/// ## Node Instance Data Access
///
/// Nodes that need per-instance state (playback time, transition progress, etc.) access it via
/// GetAnimNodeInstanceData<T>(). The instance data offset is pre-computed during graph preparation
/// and stored in the node.
///
/// **Example from a node's Step() method:**
/// ```cpp
/// InstanceData* pState = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);
/// pState->m_PlaybackTime += tDiff;
/// ```
class EZ_RENDERERCORE_DLL ezAnimGraphInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraphInstance);

public:
  ezAnimGraphInstance();
  ~ezAnimGraphInstance();

  /// Allocates instance data and initializes pin state pointers for the given graph.
  ///
  /// Must be called once after construction. The graph must have been prepared via PrepareForUse().
  void Configure(const ezAnimGraph& animGraph);

  /// Executes all nodes in the graph to generate animation output.
  void Update(ezAnimController& ref_controller, ezTime diff, ezGameObject* pTarget, const ezSkeletonResource* pSekeltonResource);

  /// Retrieves the instance data for a specific node.
  ///
  /// Nodes use this to access their per-instance state (playback time, blend weights, etc.).
  /// The type T should match the InstanceData struct defined in the node class.
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
  friend class ezAnimGraphNumberInputPin;
  friend class ezAnimGraphNumberOutputPin;
  friend class ezAnimGraphBoolInputPin;
  friend class ezAnimGraphBoolOutputPin;
};
