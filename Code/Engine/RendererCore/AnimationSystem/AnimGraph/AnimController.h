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

/// Runtime data for bone weight pins, controlling which bones are affected by animations.
///
/// Used to mask out specific bones from animation influence, allowing selective animation blending.
struct ezAnimGraphPinDataBoneWeights
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const ezAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

/// Runtime data for local pose pins, containing bone transforms in local space (relative to parent).
///
/// Local poses are the output of pose sampling and blending nodes before forward kinematics is applied.
struct ezAnimGraphPinDataLocalTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  const ezAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  bool m_bUseRootMotion = false;
};

/// Runtime data for model pose pins, containing bone transforms in model space (relative to skeleton root).
///
/// Model poses are the output after forward kinematics has been applied, ready for final rendering.
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

/// Manages and updates animation graph instances to generate animation poses.
///
/// This is the runtime bridge between animation graphs (ezAnimGraph) and the pose generation system
/// (ezAnimPoseGenerator). It owns graph instances, manages their execution, handles data flow between
/// graph nodes, and forwards the final pose to the renderer.
///
/// ## Responsibilities
///
/// - Creates and updates an ezAnimGraphInstance object for each loaded graph
/// - Allocates and manages pin data storage (bone weights, local poses, model poses)
/// - Accumulates root motion from animations for character movement
/// - Forwards the provided blackboard for data sharing between graph nodes and gameplay code
/// - Maps animation clip names to actual animation clip resources
/// - Manages shared bone weight masks across all characters
///
/// ## Usage Pattern
///
/// Typically owned by ezAnimationControllerComponent. The component calls Initialize() once,
/// then Update() every frame to generate new animation poses.
///
/// **Basic workflow:**
/// ```cpp
/// ezAnimController controller;
/// controller.Initialize(skeletonHandle, poseGenerator, blackboard);
/// controller.AddAnimGraph(animGraphHandle);
/// controller.SetAnimationClipInfo("Walk", walkClipInfo);
///
/// // Each frame:
/// controller.Update(deltaTime, pGameObject, bEnableIK);
/// controller.GetRootMotion(translation, rotX, rotY, rotZ);
/// ```
///
/// ## Blackboard Integration
///
/// The blackboard is used for communication between gameplay code and animation graphs.
/// Nodes can read blackboard values (movement speed, aim direction) to drive blending,
/// and write values (footstep events, animation state) back to gameplay.
///
/// ## Multiple Graphs
///
/// Multiple animation graphs can be added to create layered animations (e.g., base locomotion
/// + upper body aim).
class EZ_RENDERERCORE_DLL ezAnimController
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimController);

public:
  ezAnimController();
  ~ezAnimController();

  /// Initializes the controller with a skeleton and pose generator.
  ///
  /// Must be called before adding graphs or updating. The optional blackboard is used for
  /// data sharing between animation graphs and gameplay code.
  void Initialize(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard = nullptr);

  /// Updates all animation graph instances and generates the final pose.
  ///
  /// Steps all nodes in all graphs, accumulates results, forwards the final pose to the pose generator,
  /// and sends ezMsgAnimationPoseUpdated to the target object and its children.
  ///
  /// Returns false if the target requested to stop animating via msg.m_bContinueAnimating.
  bool Update(ezTime diff, ezGameObject* pTarget, bool bEnableIK);

  /// Retrieves accumulated root motion from the last Update().
  ///
  /// Root motion is the translation and rotation extracted from animations, used to move characters
  /// based on their animation. This could be applied to the character controller or entity.
  void GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const;

  const ezSharedPtr<ezBlackboard>& GetBlackboard() { return m_pBlackboard; }

  ezAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  /// Creates or retrieves a shared bone weight mask.
  ///
  /// Bone weights are shared globally across all characters to save memory. The fill delegate is called
  /// only once when the weights are first created. Subsequent calls with the same name return the cached weights.
  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);

  void AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms);

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  /// Loads an animation graph and creates a runtime instance for it.
  ///
  /// Multiple graphs can be added to layer animations. They are evaluated in the order added.
  void AddAnimGraph(const ezAnimGraphResourceHandle& hGraph);

  struct AnimClipInfo
  {
    ezAnimationClipResourceHandle m_hClip;
  };

  const AnimClipInfo& GetAnimationClipInfo(ezTempHashedString sClipName) const;

  /// Sets which animation clip is used for the named animation.
  ///
  /// Should only be called right at the start or when it is absolutely certain that an animation clip isn't in use right now,
  /// otherwise the running animation playback may produce weird results.
  void SetAnimationClipInfo(const ezHashedString& sClipName, const AnimClipInfo& info);

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
  friend class ezAnimGraphNumberInputPin;
  friend class ezAnimGraphNumberOutputPin;
  friend class ezAnimGraphBoolInputPin;
  friend class ezAnimGraphBoolOutputPin;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;
};
