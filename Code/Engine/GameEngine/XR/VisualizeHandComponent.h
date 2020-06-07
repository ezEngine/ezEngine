#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class ezVisualizeHandComponent, ezComponentUpdateType::WhenSimulating> ezVisualizeHandComponentManager;

class EZ_GAMEENGINE_DLL ezVisualizeHandComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualizeHandComponent, ezComponent, ezVisualizeHandComponentManager);
public:
  ezVisualizeHandComponent();
  ~ezVisualizeHandComponent();

protected:
  void Update();
};
