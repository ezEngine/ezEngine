#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezTwoBoneIKComponentManager = ezComponentManager<class ezTwoBoneIKComponent, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezTwoBoneIKComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTwoBoneIKComponent, ezComponent, ezTwoBoneIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTwoBoneIKComponent

public:
  ezTwoBoneIKComponent();
  ~ezTwoBoneIKComponent();

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]
};
