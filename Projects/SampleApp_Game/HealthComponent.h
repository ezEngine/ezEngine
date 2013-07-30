#pragma once

#include <Core/World/World.h>
#include <DamageComponent.h>

class HealthComponent;
typedef ezComponentManagerSimple<HealthComponent> HealthComponentManager;

//class HealthMessage : public ezMessage
//{
//  EZ_DECLARE_MESSAGE_TYPE(HealthMessage);
//
//  float m_fHealthInc;
//};

void RenderProjectile(ezGameObject* pObj, HealthComponent* pProjectile);

class HealthComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(HealthComponent, HealthComponentManager);

public:
  HealthComponent()
  {
    //m_fHealth = 100.0f;
    m_vVelocity.SetZero();
    m_iTimeToLive = 25;
    m_iBelongsToPlayer = -1;
    m_bDoesDamage = false;
    m_bDeleted = false;
  }

  void Update();

  //float GetHealth() const { return m_fHealth; }
  //
  //void SetHealth(float fHealth)
  //{
  //  m_fHealth = fHealth;
  //  if (m_fHealth <= 0.0f)
  //  {
  //    // dead: do something spectacular
  //    GetWorld()->DeleteObject(m_pOwner->GetHandle());
  //  }
  //}

  //virtual void OnMessage(ezMessage& msg) EZ_OVERRIDE
  //{
  //  if (msg.GetId() == HealthMessage::MSG_ID)
  //  {
  //    SetHealth(m_fHealth + static_cast<HealthMessage&>(msg).m_fHealthInc);
  //  }
  //}

  //float m_fHealth;



  ezInt32 m_iTimeToLive;
  ezVec3 m_vVelocity;
  ezInt32 m_iBelongsToPlayer;
  bool m_bDoesDamage;
  bool m_bDeleted;
};

//EZ_IMPLEMENT_MESSAGE_TYPE(HealthMessage);
EZ_IMPLEMENT_COMPONENT_TYPE(HealthComponent, HealthComponentManager);
