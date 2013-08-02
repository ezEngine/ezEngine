#include "Level.h"
#include "AsteroidComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(AsteroidComponent, AsteroidComponentManager);

AsteroidComponent::AsteroidComponent()
{
  m_fRadius = 1.0f;
  m_iShapeRandomSeed = rand() % 1000;
  m_fRotationSpeed = ((rand() % 1000) / 999.0f) * 2.0f - 1.0f ;
}

void AsteroidComponent::Update()
{
  const Level* pLevel = (Level*) GetWorld()->m_pUserData;

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_fRotationSpeed);
  
  m_pOwner->SetLocalRotation(qRot * m_pOwner->GetLocalRotation());
}
