#include "Level.h"
#include "ProjectileComponent.h"
#include "ShipComponent.h"
#include "CollidableComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(ProjectileComponent, ProjectileComponentManager);

ProjectileComponent::ProjectileComponent()
{
  m_vVelocity.SetZero();
  m_iTimeToLive = 25;
  m_iBelongsToPlayer = -1;
  m_bDoesDamage = false;
  m_vDrawDir.SetZero();
}

void ProjectileComponent::Update()
{
  if (m_iTimeToLive <= 0)
  {
    /// \todo Clemens: this does not work -> can't spawn more components afterwards (are they reused badly?)
    //GetWorld()->DeleteObject(m_pOwner->GetHandle());
    return;
  }

  --m_iTimeToLive;

  if (m_vVelocity.IsZero())
    return;

  m_vDrawDir = m_vVelocity;

  const Level* pLevel = (Level*) GetWorld()->m_pUserData;

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);

  CollidableComponentManager* pCollidableManager = GetWorld()->GetComponentManager<CollidableComponentManager>();

  ezBlockStorage<CollidableComponent>::Iterator it = pCollidableManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    CollidableComponent& Collider = *it;
    ezGameObject* pColliderObject = Collider.GetOwner();
    ShipComponent* pShipComponent = pColliderObject->GetComponentOfType<ShipComponent>();

    if (pShipComponent)
    {
      if (pShipComponent->m_iPlayerIndex == m_iBelongsToPlayer)
        continue;

      if (pShipComponent->m_iHealth <= 0)
        continue;
    }

    ezBoundingSphere bs(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

    const ezVec3 vPos = m_pOwner->GetLocalPosition();

    if (!m_vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
    {
      if (pShipComponent && m_bDoesDamage)
      {
        pShipComponent->m_iHealth = ezMath::Max(pShipComponent->m_iHealth - 100, 0);

        const ezInt32 iMaxParticles = 100;

        if (false)
        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), /*180.0f +*/ (float) (i - (iMaxParticles / 2)) * fSteps);
            const ezVec3 vDir = qRot * m_vVelocity;

            {
              ezGameObjectDesc desc;
              desc.m_LocalPosition = m_pOwner->GetLocalPosition();
              ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

              ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);
              pProjectile->AddComponent(GetWorld()->GetComponentManager<ProjectileComponentManager>()->CreateComponent());

              ProjectileComponent* pProjectileComponent = pProjectile->GetComponentOfType<ProjectileComponent>();
              pProjectileComponent->m_iBelongsToPlayer = m_iBelongsToPlayer;
              pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
              pProjectileComponent->m_bDoesDamage = false;
            }
          }
        }

        //if (false)
        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), /*180.0f +*/ (float) (i - (iMaxParticles / 2)) * fSteps);
            const ezVec3 vDir = qRot * m_vVelocity;

            {
              ezGameObjectDesc desc;
              desc.m_LocalPosition = m_pOwner->GetLocalPosition();
              ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

              ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);
              pProjectile->AddComponent(GetWorld()->GetComponentManager<ProjectileComponentManager>()->CreateComponent());

              ProjectileComponent* pProjectileComponent = pProjectile->GetComponentOfType<ProjectileComponent>();
              pProjectileComponent->m_iBelongsToPlayer = pShipComponent->m_iPlayerIndex;
              pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
              pProjectileComponent->m_bDoesDamage = false;
            }
          }
        }
      }

      if (pShipComponent)
        m_iTimeToLive = 0;
      else
      {
        m_vDrawDir = m_vVelocity;
        m_vDrawDir.SetLength(0.5f);
        m_vVelocity.SetZero();
        m_iTimeToLive = 150;
      }
    }
  }
}