#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Defines the structure and logic of an animation graph as a directed acyclic graph (DAG) of nodes.
///
/// An animation graph consists of interconnected nodes that process animation data. Nodes can sample animation clips,
/// blend poses, handle logic and math operations, react to events, and output final poses. Data flows through
/// typed pins that connect nodes together.
///
/// ## Architecture
///
/// **Graph Definition vs Instance:**
/// - ezAnimGraph is the shared definition (nodes, connections, structure)
/// - ezAnimGraphInstance is per-character runtime state (playback time, blend weights, pin values)
/// - One graph can be shared by hundreds of characters, each with their own instance
///
/// **Node Types:**
/// - Pose sampling: Sample animation clips, blend spaces, sequences
/// - Blending: Lerp, switch between poses, blend spaces (1D/2D)
/// - Logic: Boolean operations, comparisons, state machines
/// - Math: Add, multiply, clamp numbers for blend weights
/// - Events: Trigger gameplay events at specific animation times
/// - Bone weights: Control which bones are affected by animations
/// - Root motion: Extract character movement from animations
/// - Output: Final pose output to renderer
///
/// **Pin Types:**
/// - Trigger: One-shot events (animation finished, state entered)
/// - Number: Blend weights, playback speeds, parameters
/// - Bool: Conditions, flags
/// - BoneWeights: Masks controlling which bones are affected
/// - LocalPose: Bone transforms in local space
/// - ModelPose: Bone transforms in model space
///
/// ## Usage Pattern
///
/// Animation graphs are typically created by the editor and stored in ezAnimGraphResource.
/// At runtime, ezAnimController is used to load a graph resource, create an ezAnimGraphInstance
/// and updates it each frame.
class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  void Clear();

  ezAnimGraphNode* AddNode(ezUniquePtr<ezAnimGraphNode>&& pNode);
  void AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, ezAnimGraphNode* pDstNode, ezStringView sDstPinName);

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  const ezInstanceDataAllocator& GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  ezArrayPtr<const ezUniquePtr<ezAnimGraphNode>> GetNodes() const { return m_Nodes; }

  /// Prepares the graph for use by sorting nodes, assigning pin indices, and allocating instance data descriptors.
  ///
  /// Must be called after all nodes and connections have been added and before creating instances.
  /// This is automatically called when deserializing a graph or when creating instances.
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
};
