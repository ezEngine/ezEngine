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

  ezHashedString m_sTargetScene;           // [ property ]
  ezHashedString m_sSpawnPoint;            // [ property ]
  bool m_bRelativeSpawnPosition = true;    // [ property ]
  ezHashedString m_sPreloadCollectionFile; // [ property ]

  void StartTransition(const ezTransform& relativePosition);

protected:
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg);
};
