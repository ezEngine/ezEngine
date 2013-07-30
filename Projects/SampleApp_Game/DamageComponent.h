#pragma once

#include <Core/World/World.h>

#include "HealthComponent.h"

class DamageComponent;
typedef ezComponentManagerSimple<DamageComponent> DamageComponentManager;

//extern ezWorld* m_pWorld;

class DamageComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DamageComponent, DamageComponentManager);

public:
  DamageComponent()
  {
    //m_fDamagePerSecond = 25.0f;
    m_vVelocity.SetZero();
    m_bIsShooting = false;
    m_iShootDelay = 0;
    m_iAmmunition = 100;
    m_iAmmoPerShot = 20;
    m_iHealth = 1000;
    m_iPlayerIndex = -1;
  }

  //float GetDamagePerSecond() const { return m_fDamagePerSecond; }
  //
  //void SetDamagePerSecond(float fDamagePerSecond) { m_fDamagePerSecond = fDamagePerSecond; }

  void Update();

  void SetVelocity(ezVec3& vVel)
  {
    m_vVelocity += vVel * 0.2f;
  }

  void SetIsShooting(bool b)
  {
    m_bIsShooting = b;
  }

  ezInt32 m_iHealth;
  ezInt32 m_iPlayerIndex;

private:
  //float m_fDamagePerSecond;
  ezVec3 m_vVelocity;
  bool m_bIsShooting;
  ezInt32 m_iShootDelay;
  ezInt32 m_iAmmunition;
  ezInt32 m_iAmmoPerShot;
};

EZ_IMPLEMENT_COMPONENT_TYPE(DamageComponent, DamageComponentManager);
