#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <PacManPlugin/PacManPluginDLL.h>

using PacManComponentManager = ezComponentManagerSimple<class PacManComponent, ezComponentUpdateType::WhenSimulating>;

class PacManComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(PacManComponent, ezComponent, PacManComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SampleBounceComponent

public:
  PacManComponent();
  ~PacManComponent();

private:
  void Update();

  void OnMsgInputActionTriggered(ezMsgInputActionTriggered& msg);
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg);

  ezInt32 m_iDirection = 0;
  ezInt32 m_iTargetDirection = 0;

  ezPrefabResourceHandle m_hCollectCoint;
  ezPrefabResourceHandle m_hLoseGame;
};
