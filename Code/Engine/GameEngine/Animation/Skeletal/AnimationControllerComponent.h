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

  void SetAnimGraphFile(const char* szFile); // [ property ]
  const char* GetAnimGraphFile() const;      // [ property ]

  ezEnum<ezAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();

  ezEnum<ezRootMotionMode> m_RootMotionMode;

  ezAnimGraphResourceHandle m_hAnimGraph;
  ezAnimController m_AnimController;
  ezAnimPoseGenerator m_PoseGenerator;

  ezTime m_ElapsedTimeSinceUpdate = ezTime::Zero();
};
