#include "Level.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "CollidableComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(ShipComponent, ShipComponentManager);

ShipComponent::ShipComponent()
{
  m_vVelocity.SetZero();
  m_bIsShooting = false;
  m_iShootDelay = 2;//5;
  m_iCurShootCooldown = 0;
  m_iMaxAmmunition = 100;
  m_iAmmunition = m_iMaxAmmunition / 2;
  m_iAmmoPerShot = 10;
  m_iHealth = 1000;
  m_iPlayerIndex = -1;
}

void ShipComponent::SetVelocity(const ezVec3& vVel)
{
  m_vVelocity += vVel * 0.1f;
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
    CollidableComponentManager* pCollidableManager = GetWorld()->GetComponentManager<CollidableComponentManager>();

    ezBlockStorage<CollidableComponent>::Iterator it = pCollidableManager->GetComponents();

    for ( ; it.IsValid(); ++it)
    {
      CollidableComponent& Collider = *it;
      ezGameObject* pColliderObject = Collider.GetOwner();
      ShipComponent* pShipComponent = pColliderObject->GetComponentOfType<ShipComponent>();

      if (pShipComponent)
      {
        if (pShipComponent->m_iPlayerIndex == m_iPlayerIndex)
          continue;

        if (pShipComponent->m_iHealth <= 0)
          continue;
      }

      ezBoundingSphere bs(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

      const ezVec3 vPos = m_pOwner->GetLocalPosition();

      if (!bs.Contains(vPos) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
      {
        m_vVelocity *= -0.5f;
      }
    }
  }

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);
  m_vVelocity *= 0.97f;

  if (m_iCurShootCooldown > 0)
    --m_iCurShootCooldown;
  else
    if (m_bIsShooting && m_iAmmunition >= m_iAmmoPerShot)
    {
      m_iCurShootCooldown = m_iShootDelay;

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

    m_iAmmunition = ezMath::Clamp(m_iAmmunition + 1, 0, m_iMaxAmmunition);
    m_iHealth = ezMath::Clamp(m_iHealth + 1, 0, 1000);


    ezVec3 vCurPos = m_pOwner->GetLocalPosition();
    vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
    vCurPos = vCurPos.CompMin(ezVec3(20.0f));

    m_pOwner->SetLocalPosition(vCurPos);
}
