#include <PCH.h>
#include "Level.h"

template<typename ComponentType, typename ManagerType>
ComponentType* AddComponent(ezWorld* pWorld, ezGameObject* pObject)
{
  ManagerType* pManager = pWorld->GetComponentManager<ManagerType>();

  ComponentType* pComponent = NULL;
  ezComponentHandle hComponent = pManager->CreateComponent(pComponent);
  
  pObject->AddComponent(hComponent);
  return pComponent;
}

ezGameObject* Level::CreateGameObject(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling)
{
  ezGameObjectDesc desc;
  desc.m_LocalPosition = vPosition;
  desc.m_LocalRotation = qRotation;
  desc.m_LocalScaling.Set(fScaling);

  ezGameObject* pObject = NULL;
  m_pWorld->CreateObject(desc, pObject);

  return pObject;
}

ezGameObjectHandle Level::CreateUnit_Default(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling)
{
  ezGameObject* pObject = CreateGameObject(vPosition, qRotation, fScaling);

  UnitComponent* pUnitComponent;

  // Unit component
  {
    pUnitComponent = AddComponent<UnitComponent, UnitComponentManager>(m_pWorld, pObject);

    pUnitComponent->SetUnitType(UnitType::Default);
  }

  // Revealer component
  {
    RevealerComponent* pComponent = AddComponent<RevealerComponent, RevealerComponentManager>(m_pWorld, pObject);

  }

  // Avoid Obstacle Steering Behavior component
  {
    AvoidObstacleSteeringComponent* pComponent = AddComponent<AvoidObstacleSteeringComponent, AvoidObstacleSteeringComponentManager>(m_pWorld, pObject);

  }

  // Follow Path Steering Behavior component
  {
    FollowPathSteeringComponent* pComponent = AddComponent<FollowPathSteeringComponent, FollowPathSteeringComponentManager>(m_pWorld, pObject);

    pComponent->SetPath(&pUnitComponent->m_Path);

  }

  return pObject->GetHandle();
}

ezGameObjectHandle Level::CreateUnit(UnitType::Enum Type, const ezVec3& vPosition, const ezQuat& qRotation, float fScaling)
{
  switch (Type)
  {
  case UnitType::Default:
    return CreateUnit_Default(vPosition, qRotation, fScaling);
  }

  EZ_REPORT_FAILURE("Unknown Unit Type %i", Type);
  return ezGameObjectHandle();
}

