#include "Level.h"
#include "ProjectileComponent.h"
#include "ShipComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(ProjectileComponent, ProjectileComponentManager);

void RenderProjectile(ezGameObject* pObj, ProjectileComponent* pProjectile);

ProjectileComponent::ProjectileComponent()
{
  m_vVelocity.SetZero();
  m_iTimeToLive = 25;
  m_iBelongsToPlayer = -1;
  m_bDoesDamage = false;
}

void ProjectileComponent::Update()
{
  if (m_iTimeToLive <= 0)
    return;

  const Level* pLevel = (Level*) GetWorld()->m_pUserData;

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);
  --m_iTimeToLive;

  RenderProjectile(m_pOwner, this);

  for (ezInt32 p = 0; p < MaxPlayers; ++p)
  {
    if (p == m_iBelongsToPlayer)
      continue;

    ezGameObject* pEnemy = GetWorld()->GetObject(pLevel->GetPlayerShip(p));
    ShipComponent* pEnemeyComponent = pEnemy->GetComponentOfType<ShipComponent>();

    if (pEnemeyComponent->m_iHealth <= 0)
      continue;

    ezBoundingSphere bs(pEnemy->GetLocalPosition(), 1.5f);

    const ezVec3 vPos = m_pOwner->GetLocalPosition();

    if (!m_vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
    {
      if (m_bDoesDamage)
      {
        pEnemeyComponent->m_iHealth = ezMath::Max(pEnemeyComponent->m_iHealth - 100, 0);

        const ezInt32 iMaxParticles = 100;

        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), 180.0f + (float) (i - (iMaxParticles / 2)) * fSteps);
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

        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), 180.0f + (float) (i - (iMaxParticles / 2)) * fSteps);
            const ezVec3 vDir = qRot * m_vVelocity;

            {
              ezGameObjectDesc desc;
              desc.m_LocalPosition = m_pOwner->GetLocalPosition();
              ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

              ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);
              pProjectile->AddComponent(GetWorld()->GetComponentManager<ProjectileComponentManager>()->CreateComponent());

              ProjectileComponent* pProjectileComponent = pProjectile->GetComponentOfType<ProjectileComponent>();
              pProjectileComponent->m_iBelongsToPlayer = pEnemeyComponent->m_iPlayerIndex;
              pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
              pProjectileComponent->m_bDoesDamage = false;
            }
          }
        }
      }

      m_iTimeToLive = 0;
    }
  }
}