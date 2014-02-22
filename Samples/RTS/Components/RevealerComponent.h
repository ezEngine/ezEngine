#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>
#include <GameUtils/DataStructures/GameGrid.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

class RevealerComponent;
typedef ezComponentManagerSimple<RevealerComponent> RevealerComponentManager;

class RevealerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(RevealerComponent, RevealerComponentManager);

public:
  RevealerComponent();

  static float g_fDefaultRadius;

private:
  static ezCallbackResult::Enum TagCellVisible(ezInt32 x, ezInt32 y, void* pPassThrough);

  // Called by RevealerComponentManager
  virtual void Update();

  bool m_bRaycasting;
  float m_fRadius;
  ezUInt8 m_uiVisibilityValue;
};

