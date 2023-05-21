#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using ezVisualizeHandComponentManager = ezComponentManagerSimple<class ezVisualizeHandComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_GAMEENGINE_DLL ezVisualizeHandComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualizeHandComponent, ezComponent, ezVisualizeHandComponentManager);

public:
  ezVisualizeHandComponent();
  ~ezVisualizeHandComponent();

protected:
  void Update();
};
