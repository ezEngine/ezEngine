#include "Level.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(ShipComponent, ShipComponentManager);

ShipComponent::ShipComponent()
{
  m_vVelocity.SetZero();
  m_bIsShooting = false;
  m_iShootDelay = 0;
  m_iAmmunition = 100;
  m_iAmmoPerShot = 20;
  m_iHealth = 1000;
  m_iPlayerIndex = -1;
}

void ShipComponent::SetVelocity(ezVec3& vVel)
{
  m_vVelocity += vVel * 0.2f;
}

void ShipComponent::SetIsShooting(bool b)
{
  m_bIsShooting = b;
}

void ShipComponent::Update()
{
  if (m_iHealth <= 0)
    return;

  const Level* pLevel = (Level*) GetWorld()->m_pUserData;

  const ezVec3 vShipDir = m_pOwner->GetLocalRotation() * ezVec3(0, 1, 0);

  if (!m_vVelocity.IsZero(0.001f))
  {
    for (ezInt32 p = 0; p < MaxPlayers; ++p)
    {
      if (p == m_iPlayerIndex)
        continue;

      ezGameObject* pEnemy = GetWorld()->GetObject(pLevel->GetPlayerShip(p));
      ShipComponent* pEnemeyComponent = pEnemy->GetComponentOfType<ShipComponent>();

      if (pEnemeyComponent->m_iHealth <= 0)
        continue;

      ezBoundingSphere bs(pEnemy->GetLocalPosition(), 1.0f);

      const ezVec3 vPos = m_pOwner->GetLocalPosition();

      if (bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
      {
        m_vVelocity *= -1;
      }
    }
  }

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);
  m_vVelocity *= 0.95f;

  if (m_iShootDelay > 0)
    --m_iShootDelay;
  else
    if (m_bIsShooting && m_iAmmunition >= m_iAmmoPerShot)
    {
      m_iShootDelay = 4;

      ezGameObjectDesc desc;
      desc.m_LocalPosition = m_pOwner->GetLocalPosition();
      ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

      ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);

      pProjectile->AddComponent(GetWorld()->GetComponentManager<ProjectileComponentManager>()->CreateComponent());

      ProjectileComponent* pProjectileComponent = pProjectile->GetComponentOfType<ProjectileComponent>();
      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_vVelocity = vShipDir * ezMath::Max(m_vVelocity.GetLength(), 1.0f) * 1.0f;
      pProjectileComponent->m_bDoesDamage = true;

      m_iAmmunition -= m_iAmmoPerShot;
    }

    m_iAmmunition = ezMath::Clamp(m_iAmmunition + 1, 0, 100);
    //m_iHealth = ezMath::Clamp(m_iHealth + 1, 0, 1000);


    ezVec3 vCurPos = m_pOwner->GetLocalPosition();
    vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
    vCurPos = vCurPos.CompMin(ezVec3(20.0f));

    m_pOwner->SetLocalPosition(vCurPos);
}
