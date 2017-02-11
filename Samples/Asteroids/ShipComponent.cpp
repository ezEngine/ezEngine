#include "Level.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "CollidableComponent.h"
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <Foundation/Configuration/CVar.h>

EZ_BEGIN_COMPONENT_TYPE(ShipComponent, 1);
EZ_END_COMPONENT_TYPE

ezCVarFloat CVar_MaxAmmo("g_MaxAmmo", 20.0f, ezCVarFlags::Default, "How much ammo a ship can store");
ezCVarFloat CVar_MaxHealth("g_MaxHealth", 30.0f, ezCVarFlags::Default, "How much health a ship can have");
ezCVarFloat CVar_ProjectileSpeed("g_ProjectileSpeed", 100.0f, ezCVarFlags::Default, "Projectile fly speed");
ezCVarFloat CVar_ProjectileAmmoPerShot("g_AmmoPerShot", 0.2f, ezCVarFlags::Default, "Ammo used up per shot");
ezCVarFloat CVar_ShotDelay("g_ShotDelay", 1.0 / 20.0, ezCVarFlags::Default, "Delay between each shot");

ShipComponent::ShipComponent()
{
  m_vVelocity.SetZero();
  m_bIsShooting = false;
  m_fAmmunition = CVar_MaxAmmo / 2.0f;
  m_fHealth = CVar_MaxHealth;
  m_iPlayerIndex = -1;
}

void ShipComponent::SetVelocity(const ezVec3& vVel)
{
  m_vVelocity += vVel * 0.1f;

  ezStringBuilder s, v;
  s.Format("Game/Player{0}/Velocity", m_iPlayerIndex);
  v.Format("{0} | {1} | {2}", ezArgF(m_vVelocity.x, 3), ezArgF(m_vVelocity.y, 3), ezArgF(m_vVelocity.z, 3));

  ezStats::SetStat(s.GetData(), v.GetData());
}

void ShipComponent::SetIsShooting(bool b)
{
  //ezInputDeviceXBox360::GetDevice()->SetVibrationStrength(m_iPlayerIndex, 0, b ? 1.0f : 0.0f);
  //ezInputDeviceXBox360::GetDevice()->SetVibrationStrength(m_iPlayerIndex, 1, b ? 1.0f : 0.0f);

  m_bIsShooting = b;

  ezStringBuilder s, v;
  s.Format("Game/Player{0}/Shooting", m_iPlayerIndex);
  v = b ? "Yes" : "No";

  ezStats::SetStat(s.GetData(), v.GetData());
}

void ShipComponent::Update()
{
  if (!IsAlive())
    return;

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (!m_vVelocity.IsZero(0.001f))
  {
    CollidableComponentManager* pCollidableManager = GetWorld()->GetOrCreateComponentManager<CollidableComponentManager>();

    for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
    {
      CollidableComponent& Collider = *it;
      ezGameObject* pColliderObject = Collider.GetOwner();
      ShipComponent* pShipComponent = nullptr;

      if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
      {
        if (pShipComponent->m_iPlayerIndex == m_iPlayerIndex)
          continue;

        if (!pShipComponent->IsAlive())
          continue;
      }

      ezBoundingSphere bs(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

      const ezVec3 vPos = GetOwner()->GetLocalPosition();

      if (!bs.Contains(vPos) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
      {
        m_vVelocity *= -0.5f;
      }
    }
  }

  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + m_vVelocity);
  m_vVelocity *= 0.97f;

  if (m_CurShootCooldown > ezTime::Seconds(0))
  {
    m_CurShootCooldown -= tDiff;
  }
  else if (m_bIsShooting && m_fAmmunition >= CVar_ProjectileAmmoPerShot)
  {
    m_CurShootCooldown = ezTime::Seconds(CVar_ShotDelay);

    ezGameObjectDesc desc;
    desc.m_LocalPosition = GetOwner()->GetLocalPosition();
    desc.m_LocalRotation = GetOwner()->GetGlobalRotation();

    ezGameObject* pProjectile = nullptr;
    ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc, pProjectile);

    {
      ProjectileComponent* pProjectileComponent = nullptr;
      ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(GetWorld(), pProjectileComponent);

      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_fSpeed = CVar_ProjectileSpeed;
      pProjectileComponent->m_bDoesDamage = true;

      pProjectile->AttachComponent(hProjectileComponent);
    }

    // ProjectileMesh
    {
      ezMeshComponent* pMeshComponent = nullptr;
      ezMeshComponent::CreateComponent(GetWorld(), pMeshComponent);

      pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ProjectileMesh"));

      // this only works because the materials are part of the Asset Collection and get a name like this from there
      // otherwise we would need to have the GUIDs of the 4 different material assets available
      ezStringBuilder sMaterialName;
      sMaterialName.Format("MaterialPlayer{0}", m_iPlayerIndex + 1);
      pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));

      pProjectile->AttachComponent(pMeshComponent);
    }

    m_fAmmunition -= CVar_ProjectileAmmoPerShot;

    float ShootTrack[20] =
    {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    ezInputDeviceXBox360::GetDevice()->AddVibrationTrack(m_iPlayerIndex, ezInputDeviceController::Motor::RightMotor, ShootTrack, 20);
  }

  m_fAmmunition = ezMath::Clamp<float>(m_fAmmunition + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxAmmo);
  m_fHealth = ezMath::Clamp<float>(m_fHealth + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxHealth);


  ezVec3 vCurPos = GetOwner()->GetLocalPosition();
  vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
  vCurPos = vCurPos.CompMin(ezVec3(20.0f));

  GetOwner()->SetLocalPosition(vCurPos);


  ezStringBuilder s, v;

  {
    s.Format("Game/Player{0}/Health", m_iPlayerIndex);
    v.Format("{0}", m_fHealth);

    ezStats::SetStat(s.GetData(), v.GetData());
  }

  {
    s.Format("Game/Player{0}/Ammo", m_iPlayerIndex);
    v.Format("{0}", m_fAmmunition);

    ezStats::SetStat(s.GetData(), v.GetData());
  }
}
