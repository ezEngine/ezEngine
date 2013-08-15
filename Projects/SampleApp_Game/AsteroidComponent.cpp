#include "Level.h"
#include "AsteroidComponent.h"
#include "ShipComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(AsteroidComponent, AsteroidComponentManager);

AsteroidComponent::AsteroidComponent()
{
  m_fRadius = 1.0f;
  m_iShapeRandomSeed = rand() % 1000;
  m_fRotationSpeed = ((rand() % 1000) / 999.0f) * 2.0f - 1.0f ;
}

void AsteroidComponent::Update()
{
  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_fRotationSpeed);
  
  m_pOwner->SetLocalRotation(qRot * m_pOwner->GetLocalRotation());

  const ezVec3 vOwnPos = m_pOwner->GetLocalPosition();

  ShipComponentManager* pShipManager = GetWorld()->GetComponentManager<ShipComponentManager>();

  ezBlockStorage<ShipComponent>::Iterator it = pShipManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    ShipComponent& Ship = *it;
    ezGameObject* pObject = Ship.GetOwner();

    const ezVec3 vDir = pObject->GetLocalPosition() - vOwnPos;

    const float fDist = vDir.GetLength();

    const float fMaxDist = 4.0f;

    if (fDist > fMaxDist)
      continue;

    const float fFactor = 1.0f - fDist / fMaxDist;

    const float fScaledFactor = ezMath::Pow(fFactor, 2.0f);

    const ezVec3 vPull = vDir * fScaledFactor;

    Ship.SetVelocity(vPull * 0.06f);

  }
}
