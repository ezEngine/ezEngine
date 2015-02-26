#include "Level.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "CollidableComponent.h"
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Utilities/Stats.h>

EZ_BEGIN_COMPONENT_TYPE(ShipComponent, ezComponent, 1, ShipComponentManager);
EZ_END_COMPONENT_TYPE();

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

  ezStringBuilder s, v;
  s.Format("Game/Player%i/Velocity", m_iPlayerIndex);
  v.Format("%.3f | %.3f | %.3f", m_vVelocity.x, m_vVelocity.y, m_vVelocity.z);

  ezStats::SetStat(s.GetData(), v.GetData());
}

void ShipComponent::SetIsShooting(bool b)
{
  //ezInputDeviceXBox360::GetDevice()->SetVibrationStrength(m_iPlayerIndex, 0, b ? 1.0f : 0.0f);
  //ezInputDeviceXBox360::GetDevice()->SetVibrationStrength(m_iPlayerIndex, 1, b ? 1.0f : 0.0f);

  m_bIsShooting = b;

  ezStringBuilder s, v;
  s.Format("Game/Player%i/Shooting", m_iPlayerIndex);
  v = b ? "Yes" : "No";

  ezStats::SetStat(s.GetData(), v.GetData());
}

void ShipComponent::Update()
{
  if (m_iHealth <= 0)
    return;

  const ezVec3 vShipDir = GetOwner()->GetLocalRotation() * ezVec3(0, 1, 0);

  if (!m_vVelocity.IsZero(0.001f))
  {
    CollidableComponentManager* pCollidableManager = GetWorld()->GetComponentManager<CollidableComponentManager>();

    for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
    {
      CollidableComponent& Collider = *it;
      ezGameObject* pColliderObject = Collider.GetOwner();
      ShipComponent* pShipComponent = nullptr;

      if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
      {
        if (pShipComponent->m_iPlayerIndex == m_iPlayerIndex)
          continue;

        if (pShipComponent->m_iHealth <= 0)
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

  if (m_iCurShootCooldown > 0)
  {
    --m_iCurShootCooldown;
  }
  else if (m_bIsShooting && m_iAmmunition >= m_iAmmoPerShot)
  {
    m_iCurShootCooldown = m_iShootDelay;

    ezGameObjectDesc desc;
    desc.m_LocalPosition = GetOwner()->GetLocalPosition();

    ezGameObject* pProjectile = nullptr;
    ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc, pProjectile);

    ProjectileComponent* pProjectileComponent = nullptr;
    ezComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(GetWorld(), pProjectileComponent);

    pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
    pProjectileComponent->m_vVelocity = vShipDir * ezMath::Max(m_vVelocity.GetLength(), 1.0f) * 1.0f;
    pProjectileComponent->m_bDoesDamage = true;

    pProjectile->AddComponent(hProjectileComponent);

    m_iAmmunition -= m_iAmmoPerShot;

    float ShootTrack[20] =
    {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    ezInputDeviceXBox360::GetDevice()->AddVibrationTrack(m_iPlayerIndex, ezInputDeviceController::Motor::RightMotor, ShootTrack, 20);
  }

  m_iAmmunition = ezMath::Clamp(m_iAmmunition + 1, 0, m_iMaxAmmunition);
  m_iHealth = ezMath::Clamp(m_iHealth + 1, 0, 1000);


  ezVec3 vCurPos = GetOwner()->GetLocalPosition();
  vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
  vCurPos = vCurPos.CompMin(ezVec3(20.0f));

  GetOwner()->SetLocalPosition(vCurPos);


  ezStringBuilder s, v;

  {
    s.Format("Game/Player%i/Health", m_iPlayerIndex);
    v.Format("%i", m_iHealth);

    ezStats::SetStat(s.GetData(), v.GetData());
  }

  {
    s.Format("Game/Player%i/Ammo", m_iPlayerIndex);
    v.Format("%i", m_iAmmunition);

    ezStats::SetStat(s.GetData(), v.GetData());
  }
}
