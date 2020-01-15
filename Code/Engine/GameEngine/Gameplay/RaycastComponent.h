
#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Messages/TriggerTriggeredMessage.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class ezRaycastComponent, ezComponentUpdateType::WhenSimulating> ezRaycastComponentManager;

class EZ_GAMEENGINE_DLL ezRaycastComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRaycastComponent, ezComponent, ezRaycastComponentManager);

public:

  ezRaycastComponent();
  ~ezRaycastComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  void SetRaycastEndObject(const char* szReference); // [ property ]


private:

  void Update();

  ezGameObjectHandle m_hRaycastEndObject;
  float m_fMaxDistance = 100.0f;

  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]


private:
  const char* DummyGetter() const { return nullptr; }

};
