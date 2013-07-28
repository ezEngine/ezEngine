#pragma once

#include <Core/World/World.h>

#include "HealthComponent.h"

class DamageComponent;
typedef ezComponentManagerSimple<DamageComponent> DamageComponentManager;

class DamageComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DamageComponent, DamageComponentManager);

public:
  DamageComponent()
  {
    m_fDamagePerSecond = 25.0f;
  }

  float GetDamagePerSecond() const { return m_fDamagePerSecond; }
  
  void SetDamagePerSecond(float fDamagePerSecond) { m_fDamagePerSecond = fDamagePerSecond; }
  
  void Update()
  {
#if 0
    if (HealthComponent* pHealthComponent = m_pOwner->GetComponentOfType<HealthComponent>())
    {
      float fHealth = pHealthComponent->GetHealth();

      fHealth -= m_fDamagePerSecond * (1.0f / 60.0f); // TODO: timer class

      pHealthComponent->SetHealth(fHealth);
    }
#else
    HealthMessage msg;
    msg.m_fHealthInc = -m_fDamagePerSecond * (1.0f / 60.0f);  // TODO: timer class

    m_pOwner->SendMessage(msg);
#endif
  }

private:
  float m_fDamagePerSecond;
};

EZ_IMPLEMENT_COMPONENT_TYPE(DamageComponent, DamageComponentManager);
