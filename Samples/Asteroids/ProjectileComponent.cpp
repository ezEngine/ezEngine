#include "Level.h"
#include "ProjectileComponent.h"
#include "ShipComponent.h"
#include "CollidableComponent.h"
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Meshes/MeshComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ProjectileComponent, 1);
EZ_END_COMPONENT_TYPE

ezCVarFloat CVar_ProjectileTimeToLive("g_ProjectileTimeToLive", 0.5f, ezCVarFlags::Default, "Projectile time to Live");
ezCVarFloat CVar_SparksTimeToLive("g_SparksTimeToLive", 3.0f, ezCVarFlags::Default, "Projectile time to fade out");
ezCVarInt CVar_SparksPerHit("g_SparksPerHit", 50, ezCVarFlags::Default, "Number of particles spawned when projectile hits a ship");
ezCVarFloat CVar_ProjectileDamage("g_ProjectileDamage", 0.0f, ezCVarFlags::Default, "How much damage a projectile makes");
ezCVarFloat CVar_SparksSpeed("g_SparksSpeed", 50.0f, ezCVarFlags::Default, "Projectile fly speed");

ProjectileComponent::ProjectileComponent()
{
  m_fSpeed = 0.0f;
  m_TimeToLive = ezTime::Seconds(CVar_ProjectileTimeToLive);
  m_iBelongsToPlayer = -1;
  m_bDoesDamage = false;
}

void ProjectileComponent::Update()
{
  if (m_TimeToLive.IsZeroOrLess())
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    return;
  }

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  m_TimeToLive -= tDiff;

  if (m_TimeToLive.IsZeroOrLess())
    return;

  if (m_fSpeed <= 0.0f)
    return;

  const ezVec3 vVelocity = GetOwner()->GetLocalRotation() * ezVec3(m_fSpeed, 0, 0);
  const ezVec3 vDistance = (float)tDiff.GetSeconds() * vVelocity;
  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + vDistance);

  // Deactivate the rest of the function, if you want to profile overall engine performance
  //if (!m_bDoesDamage)
  //  return;

  CollidableComponentManager* pCollidableManager = GetWorld()->GetOrCreateComponentManager<CollidableComponentManager>();

  for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
  {
    CollidableComponent& Collider = *it;
    ezGameObject* pColliderObject = Collider.GetOwner();
    ShipComponent* pShipComponent = nullptr;

    if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
    {
      if (pShipComponent->m_iPlayerIndex == m_iBelongsToPlayer)
        continue;

      if (!pShipComponent->IsAlive())
        continue;
    }

    ezBoundingSphere bs(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

    const ezVec3 vPos = GetOwner()->GetLocalPosition();

    if (!vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + vDistance))
    {
      if (pShipComponent && m_bDoesDamage)
      {
        pShipComponent->m_fHealth = ezMath::Max(pShipComponent->m_fHealth - CVar_ProjectileDamage, 0.0f);

        {
          float HitTrack[20] =
          {
            1.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
          };

          ezInputDeviceXBox360::GetDevice()->AddVibrationTrack(pShipComponent->m_iPlayerIndex, ezInputDeviceController::Motor::LeftMotor, HitTrack, 20);
        }

        const float fAngle = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(10.0, 100.0);
        const float fSteps = fAngle / CVar_SparksPerHit;

        for (ezInt32 i = 0; i < CVar_SparksPerHit; ++i)
        {
          ezQuat qRot;
          qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree((i - (CVar_SparksPerHit / 2)) * fSteps));

          {
            ezGameObjectDesc desc;
            desc.m_LocalPosition = GetOwner()->GetLocalPosition();
            desc.m_LocalRotation = qRot * GetOwner()->GetLocalRotation();

            ezGameObject* pProjectile = nullptr;
            ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc, pProjectile);

            ProjectileComponent* pProjectileComponent = nullptr;
            ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(GetWorld(), pProjectileComponent);

            pProjectileComponent->m_iBelongsToPlayer = pShipComponent->m_iPlayerIndex;
            pProjectileComponent->m_fSpeed = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(1.0, 2.0) * CVar_SparksSpeed;
            pProjectileComponent->m_bDoesDamage = false;

            // ProjectileMesh
            {
              ezMeshComponent* pMeshComponent = nullptr;
              ezMeshComponent::CreateComponent(GetWorld(), pMeshComponent);

              pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ProjectileMesh"));

              // this only works because the materials are part of the Asset Collection and get a name like this from there
              // otherwise we would need to have the GUIDs of the 4 different material assets available
              ezStringBuilder sMaterialName;
              sMaterialName.Format("MaterialPlayer{0}", pShipComponent->m_iPlayerIndex + 1);
              pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));

              pProjectile->AttachComponent(pMeshComponent);
            }

            pProjectile->AttachComponent(hProjectileComponent);
          }
        }
      }

      if (pShipComponent)
      {
        m_TimeToLive = ezTime::Seconds(0);
      }
      else
      {
        m_fSpeed = 0.0f;
        m_TimeToLive = ezTime::Seconds(CVar_SparksTimeToLive);
      }
    }
  }
}
