#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/System/ControllerInput.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ShipComponent, 1, ezComponentMode::Dynamic);
EZ_END_COMPONENT_TYPE
// clang-format on

ezCVarFloat CVar_MaxAmmo("g_MaxAmmo", 20.0f, ezCVarFlags::Default, "How much ammo a ship can store");
ezCVarFloat CVar_MaxHealth("g_MaxHealth", 30.0f, ezCVarFlags::Default, "How much health a ship can have");
ezCVarFloat CVar_ProjectileSpeed("g_ProjectileSpeed", 100.0f, ezCVarFlags::Default, "Projectile fly speed");
ezCVarFloat CVar_ProjectileAmmoPerShot("g_AmmoPerShot", 0.2f, ezCVarFlags::Default, "Ammo used up per shot");
ezCVarFloat CVar_ShotDelay("g_ShotDelay", 1.0f / 20.0f, ezCVarFlags::Default, "Delay between each shot");

ShipComponent::ShipComponent()
{
  m_fAmmunition = CVar_MaxAmmo / 2.0f;
  m_fHealth = CVar_MaxHealth;
}

void ShipComponent::AddExternalForce(const ezVec3& vForce)
{
  m_vExternalForce += vForce;
}

void ShipComponent::SetIsShooting(bool b)
{
  m_bIsShooting = b;
}

void ShipComponent::Explode()
{
  const ezUInt32 uiNumSparks = 100;
  const float fSparksSpeed = 25.0f;

  const float fSteps = 360.0f / uiNumSparks;

  for (ezInt32 i = 0; i < uiNumSparks; ++i)
  {
    ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(i * fSteps));

    {
      ezGameObjectDesc desc;
      desc.m_bDynamic = true;
      desc.m_LocalPosition = GetOwner()->GetLocalPosition();
      desc.m_LocalRotation = qRot * GetOwner()->GetLocalRotation();

      ezGameObject* pProjectile = nullptr;
      GetWorld()->CreateObject(desc, pProjectile);

      ProjectileComponent* pProjectileComponent = nullptr;
      ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(pProjectile, pProjectileComponent);

      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_fSpeed = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(1.0, 2.0) * fSparksSpeed;
      pProjectileComponent->m_fDoesDamage = 0.75f;

      // ProjectileMesh
      {
        ezMeshComponent* pMeshComponent = nullptr;
        ezMeshComponent::CreateComponent(pProjectile, pMeshComponent);

        pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ProjectileMesh"));

        // this only works because the materials are part of the Asset Collection and get a name like this from there
        // otherwise we would need to have the GUIDs of the 4 different material assets available
        ezStringBuilder sMaterialName;
        sMaterialName.SetFormat("MaterialPlayer{0}", m_iPlayerIndex + 1);
        pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));
      }
    }
  }
}

void ShipComponent::Update()
{
  if (!IsAlive())
  {
    Explode();

    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    return;
  }


  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  // slow down the ship over time
  m_vVelocity *= ezMath::Pow(0.5f, tDiff.AsFloatInSeconds());
  // apply the external force to the ship's velocity
  m_vVelocity += tDiff.AsFloatInSeconds() * m_vExternalForce;
  // reset the forces, they will be re-added during the next update
  m_vExternalForce.SetZero();

  ezVec3 vTravelDist = m_vVelocity * tDiff.AsFloatInSeconds();

  // if the ship is at least slightly moving, do collision checks
  if (!m_vVelocity.IsZero(0.001f))
  {
    CollidableComponentManager* pCollidableManager = GetWorld()->GetOrCreateComponentManager<CollidableComponentManager>();

    for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
    {
      if (!it->IsActiveAndSimulating())
        continue;

      CollidableComponent& Collider = *it;
      ezGameObject* pColliderObject = Collider.GetOwner();
      ShipComponent* pShipComponent = nullptr;

      if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
      {
        // don't collide with yourself
        if (pShipComponent->m_iPlayerIndex == m_iPlayerIndex)
          continue;
      }

      ezBoundingSphere bs = ezBoundingSphere::MakeFromCenterAndRadius(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

      const ezVec3 vPos = GetOwner()->GetLocalPosition();

      if (!bs.Contains(vPos) && bs.GetLineSegmentIntersection(vPos, vPos + vTravelDist))
      {
        // bounce the ship into the opposite direction and lose a lot of velocity, when there is a collision
        m_vVelocity *= -0.5f;
        vTravelDist.SetZero();
        break;
      }
    }
  }

  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + vTravelDist);

  if (m_CurShootCooldown > ezTime::MakeFromSeconds(0))
  {
    m_CurShootCooldown -= tDiff;
  }
  else if (m_bIsShooting && m_fAmmunition >= CVar_ProjectileAmmoPerShot)
  {
    m_CurShootCooldown = ezTime::MakeFromSeconds(CVar_ShotDelay);

    ezGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_LocalPosition = GetOwner()->GetLocalPosition();
    desc.m_LocalRotation = GetOwner()->GetGlobalRotation();

    ezGameObject* pProjectile = nullptr;
    GetWorld()->CreateObject(desc, pProjectile);

    {
      ProjectileComponent* pProjectileComponent = nullptr;
      ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(pProjectile, pProjectileComponent);

      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_fSpeed = CVar_ProjectileSpeed;
      pProjectileComponent->m_fDoesDamage = 1.0f;
    }

    // ProjectileMesh
    {
      ezMeshComponent* pMeshComponent = nullptr;
      ezMeshComponent::CreateComponent(pProjectile, pMeshComponent);

      pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ProjectileMesh"));

      // this only works because the materials are part of the Asset Collection and get a name like this from there
      // otherwise we would need to have the GUIDs of the 4 different material assets available
      ezStringBuilder sMaterialName;
      sMaterialName.SetFormat("MaterialPlayer{0}", m_iPlayerIndex + 1);
      pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));
    }

    m_fAmmunition -= CVar_ProjectileAmmoPerShot;

    float ShootTrack[20] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    if (ezControllerInput::HasDevice())
    {
      ezControllerInput::GetDevice()->AddVibrationTrack(static_cast<ezUInt8>(m_iPlayerIndex), ezInputDeviceController::Motor::RightMotor, ShootTrack, 20);
    }
  }

  m_fAmmunition = ezMath::Clamp<float>(m_fAmmunition + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxAmmo);
  m_fHealth = ezMath::Clamp<float>(m_fHealth + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxHealth);

  // clamp the player position to the playing field
  ezVec3 vCurPos = GetOwner()->GetLocalPosition();
  vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
  vCurPos = vCurPos.CompMin(ezVec3(20.0f));

  GetOwner()->SetLocalPosition(vCurPos);
}
