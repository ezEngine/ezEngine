#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

using ezAnimationControllerComponentManager = ezComponentManagerSimple<class ezAnimationControllerComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

/// \brief Evaluates an ezAnimGraphResource and provides the result through the ezMsgAnimationPoseUpdated.
///
/// ezAnimGraph's contain logic to generate an animation pose. This component decides when it is necessary
/// to reevaluate the state, which mostly means it tracks when the object is visible.
///
/// The result is sent as a recursive message, which is usually consumed by an ezAnimatedMeshComponent.
/// The mesh component may be on the same game object or a child object.
class EZ_GAMEENGINE_DLL ezAnimationControllerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimationControllerComponent, ezComponent, ezAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimationControllerComponent

public:
  ezAnimationControllerComponent();
  ~ezAnimationControllerComponent();

  /// \brief How often to update the animation while the animated mesh is invisible.
  ezEnum<ezAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

  /// \brief If enabled, child game objects can add IK computation commands to influence the final pose.
  bool m_bEnableIK = false; // [ property ]

protected:
  void Update();

  ezEnum<ezRootMotionMode> m_RootMotionMode;

  ezAnimGraphResourceHandle m_hAnimGraph;
  ezAnimController m_AnimController;
  ezAnimPoseGenerator m_PoseGenerator;

  ezTime m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
};
