#pragma once

#include <Core/World/World.h>

class HealthComponent;
typedef ezComponentManager<HealthComponent> HealthComponentManager;

class HealthMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(HealthMessage);

  float m_fHealthInc;
};

class HealthComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(HealthComponent, HealthComponentManager);

public:
  HealthComponent()
  {
    m_fHealth = 100.0f;
  }

  float GetHealth() const { return m_fHealth; }
  
  void SetHealth(float fHealth)
  {
    m_fHealth = fHealth;
    if (m_fHealth <= 0.0f)
    {
      // dead: do something spectacular
      GetWorld()->DeleteObject(m_pOwner->GetHandle());
    }
  }

  virtual void OnMessage(ezMessage& msg) EZ_OVERRIDE
  {
    if (msg.GetId() == HealthMessage::MSG_ID)
    {
      SetHealth(m_fHealth + static_cast<HealthMessage&>(msg).m_fHealthInc);
    }
  }

  float m_fHealth;
};

EZ_IMPLEMENT_MESSAGE_TYPE(HealthMessage);
EZ_IMPLEMENT_COMPONENT_TYPE(HealthComponent, HealthComponentManager);
