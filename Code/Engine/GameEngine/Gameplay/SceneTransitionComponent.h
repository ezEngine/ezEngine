#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgTriggerTriggered;

using ezSceneTransitionComponentManager = ezComponentManager<class ezSceneTransitionComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezSceneTransitionComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSceneTransitionComponent, ezComponent, ezSceneTransitionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSceneTransitionComponent

public:
  ezSceneTransitionComponent();
  ~ezSceneTransitionComponent();

  void StartTransition();

protected:
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg);

  ezHashedString m_sTargetScene;
};
