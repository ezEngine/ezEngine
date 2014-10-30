#include "Level.h"
#include "AsteroidComponent.h"
#include "ShipComponent.h"
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Clock.h>

EZ_BEGIN_COMPONENT_TYPE(AsteroidComponent, ezComponent, 1, AsteroidComponentManager);
EZ_END_COMPONENT_TYPE();

ezCVarFloat CVar_AsteroidMaxDist("g_AsteroidMaxDist", 4.0f, ezCVarFlags::Default, "cvar float");
ezCVarFloat CVar_AsteroidPush("g_AsteroidPush", 0.06f, ezCVarFlags::Default, "cvar float");

AsteroidComponent::AsteroidComponent()
{
  m_fRotationSpeed = ((rand() % 1000) / 999.0f) * 2.0f - 1.0f;
}

void AsteroidComponent::Update()
{
  const float fTimeDiff = (float)ezClock::Get()->GetTimeDiff().GetSeconds();

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Radian(m_fRotationSpeed * fTimeDiff));
  
  GetOwner()->SetLocalRotation(qRot * GetOwner()->GetLocalRotation());

  const ezVec3 vOwnPos = GetOwner()->GetLocalPosition();

  ShipComponentManager* pShipManager = GetWorld()->GetComponentManager<ShipComponentManager>();

  for (auto it = pShipManager->GetComponents(); it.IsValid(); ++it)
  {
    ShipComponent& Ship = *it;
    ezGameObject* pObject = Ship.GetOwner();

    const ezVec3 vDir = pObject->GetLocalPosition() - vOwnPos;
    const float fDist = vDir.GetLength();
    const float fMaxDist = CVar_AsteroidMaxDist;

    if (fDist > fMaxDist)
      continue;

    const float fFactor = 1.0f - fDist / fMaxDist;
    const float fScaledFactor = ezMath::Pow(fFactor, 2.0f);
    const ezVec3 vPull = vDir * fScaledFactor;

    Ship.SetVelocity(vPull * (float) CVar_AsteroidPush);

  }
}
