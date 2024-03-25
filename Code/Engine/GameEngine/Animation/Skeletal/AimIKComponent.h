#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using ezAimIKComponentManager = ezComponentManager<class ezAimIKComponent, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezAimIKComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAimIKComponent, ezComponent, ezAimIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAimIKComponent

public:
  ezAimIKComponent();
  ~ezAimIKComponent();

protected:
  void OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const; // [ msg handler ]
};
