#include <GameUtils/PCH.h>
#include <GameUtils/Components/ProjectileComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Collision Layer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::OrangeRed)
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezProjectileComponent::ezProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
}


void ezProjectileComponent::Update()
{
  ezPhysicsWorldModuleInterface* pModule = static_cast<ezPhysicsWorldModuleInterface*>(GetManager()->GetUserData());

  if (pModule)
  {
    ezGameObject* pEntity = GetOwner();

    const float fDistance = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * m_fMetersPerSecond;

    ezVec3 vNewPosition;
    ezVec3 vCurDirection = pEntity->GetGlobalRotation() * ezVec3(1, 0, 0);

    ezVec3 vPos, vNormal;
    ezGameObjectHandle hObject;
    if (pModule->CastRay(pEntity->GetGlobalPosition(), vCurDirection, fDistance, m_uiCollisionLayer, vPos, vNormal, hObject))
    {
      const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

      //GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());

      vNewPosition = pEntity->GetGlobalPosition();// vPos;

      const ezVec3 vNewDirection = vCurDirection.GetReflectedVector(vNormal);

      ezQuat qRot;
      qRot.SetShortestRotation(vCurDirection, vNewDirection);

      GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());
    }
    else
    {
      vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
    }

    GetOwner()->SetGlobalPosition(vNewPosition);
  }
}

void ezProjectileComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_uiCollisionLayer;
}

void ezProjectileComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_uiCollisionLayer;
}


ezProjectileComponentManager::ezProjectileComponentManager(ezWorld* pWorld)
  : ezComponentManagerSimple<class ezProjectileComponent, true>(pWorld)
{

}

void ezProjectileComponentManager::Initialize()
{
  ezComponentManagerSimple<class ezProjectileComponent, true>::Initialize();

  ezPhysicsWorldModuleInterface* pModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));

  SetUserData(pModule);
}
