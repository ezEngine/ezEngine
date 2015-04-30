#pragma once

#include <Core/World/World.h>

class ShipComponent;
typedef ezComponentManagerSimple<ShipComponent> ShipComponentManager;

class ShipComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ShipComponent, ShipComponentManager);

public:
  ShipComponent();

  void Update();

  void SetVelocity(const ezVec3& vVel);

  void SetIsShooting(bool b);

  ezInt32 m_iHealth;
  ezInt32 m_iPlayerIndex;

private:
  ezVec3 m_vVelocity;
  bool m_bIsShooting;
  ezInt32 m_iShootDelay;
  ezInt32 m_iCurShootCooldown;
  ezInt32 m_iAmmunition;
  ezInt32 m_iMaxAmmunition;
  ezInt32 m_iAmmoPerShot;
};


