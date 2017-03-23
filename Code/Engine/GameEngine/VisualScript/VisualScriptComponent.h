#pragma once

#include <Core/World/World.h>

class ezVisualScriptInstance;

typedef ezComponentManagerSimple<class ezVisualScriptComponent, ezComponentUpdateType::WhenSimulating> ezVisualScriptComponentManager;

class ezVisualScriptComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualScriptComponent, ezComponent, ezVisualScriptComponentManager);

public:
  ezVisualScriptComponent();
  ~ezVisualScriptComponent();

  void Update();

protected:
  virtual bool OnUnhandledMessage(ezMessage& msg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg) const override;

  ezUniquePtr<ezVisualScriptInstance> m_Script;
};


