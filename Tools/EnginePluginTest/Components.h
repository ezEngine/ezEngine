#pragma once

#include <EnginePluginTest/EnginePluginTest.h>
#include <Core/World/World.h>

class ShipComponent;
typedef ezComponentManagerSimple<ShipComponent> ShipComponentManager;

class ShipComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ShipComponent, ShipComponentManager);

  void Update() { }

public:
  ShipComponent();

  ezInt32 m_iHealth;
  ezInt32 m_iShootDelay;
  ezInt32 m_iMaxAmmunition;
  ezInt32 m_iAmmoPerShot;
};

