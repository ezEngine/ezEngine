#include "Level.h"
#include "ProjectileComponent.h"
#include "ShipComponent.h"
#include "CollidableComponent.h"
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>

EZ_BEGIN_COMPONENT_TYPE(ProjectileComponent, ezComponent, 1, ProjectileComponentManager);
EZ_END_COMPONENT_TYPE();

ezCVarBool CVar_Test2("g_Bool", true, ezCVarFlags::Default, "cvar bool");
ezCVarInt CVar_ProjectileTTL("g_ProjectileTTL", 25, ezCVarFlags::Default, "cvar int");
ezCVarString CVar_Test4("g_String", "zwei und vierzig", ezCVarFlags::Default, "cvar string");

ProjectileComponent::ProjectileComponent()
{
  m_vVelocity.SetZero();
  m_iTimeToLive = CVar_ProjectileTTL;
  m_iBelongsToPlayer = -1;
  m_bDoesDamage = false;
  m_vDrawDir.SetZero();
}

void ProjectileComponent::Update()
{
  if (m_iTimeToLive <= 0)
  {
    GetWorld()->DeleteObject(GetOwner()->GetHandle());
    return;
  }

  --m_iTimeToLive;

  if (m_vVelocity.IsZero())
    return;

  ezStats::SetStat("g_Bool", (bool) CVar_Test2 ? "true" : "false");
  ezStats::SetStat("g_String", CVar_Test4.GetValue().GetData());

  m_vDrawDir = m_vVelocity;

  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + m_vVelocity);

  CollidableComponentManager* pCollidableManager = GetWorld()->GetComponentManager<CollidableComponentManager>();

  for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
  {
    CollidableComponent& Collider = *it;
    ezGameObject* pColliderObject = Collider.GetOwner();
    ShipComponent* pShipComponent = nullptr;

    if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
    {
      if (pShipComponent->m_iPlayerIndex == m_iBelongsToPlayer)
        continue;

      if (pShipComponent->m_iHealth <= 0)
        continue;
    }

    ezBoundingSphere bs(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

    const ezVec3 vPos = GetOwner()->GetLocalPosition();

    if (!m_vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
    {
      if (pShipComponent && m_bDoesDamage)
      {
        pShipComponent->m_iHealth = ezMath::Max(pShipComponent->m_iHealth - 100, 0);

        {
          float HitTrack[20] =
          {
            1.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
          };

          ezInputDeviceXBox360::GetDevice()->AddVibrationTrack(pShipComponent->m_iPlayerIndex, ezInputDeviceController::Motor::LeftMotor, HitTrack, 20);
        }

        const ezInt32 iMaxParticles = 100;

        const float fAngle = 10.0f + (rand() % 90);
        const float fSteps = fAngle / iMaxParticles;
        
        for (ezInt32 i = 0; i < iMaxParticles; ++i)
        {
          ezQuat qRot;
          qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), /*180.0f +*/ ezAngle::Degree((i - (iMaxParticles / 2)) * fSteps));
          const ezVec3 vDir = qRot * m_vVelocity;

          {
            ezGameObjectDesc desc;
            desc.m_LocalPosition = GetOwner()->GetLocalPosition();

            ezGameObject* pProjectile = nullptr;
            ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc, pProjectile);

            ProjectileComponent* pProjectileComponent = nullptr;
            ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(GetWorld(), pProjectileComponent);

            pProjectileComponent->m_iBelongsToPlayer = pShipComponent->m_iPlayerIndex;
            pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
            pProjectileComponent->m_bDoesDamage = false;

            pProjectile->AddComponent(hProjectileComponent);
          }
        }
      }

      if (pShipComponent)
      {
        m_iTimeToLive = 0;
      }
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