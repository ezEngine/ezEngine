#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

class ezAnimationControllerComponentManager : public ezComponentManager<class ezAnimationControllerComponent, ezBlockStorageType::FreeList>
{
public:
  ezAnimationControllerComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
  void ResourceEvent(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_ComponentsToReset;
};

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

  /// How often to update the animation while the animated mesh is invisible.
  ezEnum<ezAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

  /// If enabled, child game objects can add IK computation commands to influence the final pose.
  bool m_bEnableIK = false; // [ property ]

  /// A list of animation clips to use instead of the default ones that are set up in the animation graph.
  ezDynamicArray<ezAnimationClipMapping> m_AnimationClipOverrides; // [ property ]

  /// Overrides which animation clip resource to use for the given animation.
  ///
  /// Should only be called right at the start or when it is absolutely certain that an animation clip isn't in use right now,
  /// otherwise the running animation playback may produce weird results.
  void SetAnimationClipOverride(ezStringView sAnimationName, ezStringView sAnimationClipResource); // [ scriptable ]

protected:
  void Update();

  ezEnum<ezRootMotionMode> m_RootMotionMode;

  ezAnimGraphResourceHandle m_hAnimGraph;
  ezAnimController m_AnimController;
  ezAnimPoseGenerator m_PoseGenerator;

  ezTime m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
};
