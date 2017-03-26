#include <PCH.h>
#include <PhysXPlugin/Components/PxTriggerComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Profiling/Profiling.h>

using namespace physx;

ezPxTriggerComponentManager::ezPxTriggerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxTriggerComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezPxTriggerComponentManager::~ezPxTriggerComponentManager()
{
}

/// \todo Do we need to move triggers around? Is this even supported?

//void ezPxTriggerComponentManager::UpdateKinematicActors()
//{
//  EZ_PROFILE("KinematicActors");
//
//  for (auto pKinematicActorComponent : m_KinematicActorComponents)
//  {
//    if (PxRigidDynamic* pActor = pKinematicActorComponent->GetActor())
//    {
//      ezGameObject* pObject = pKinematicActorComponent->GetOwner();
//
//      pObject->UpdateGlobalTransform();
//
//      const ezVec3 pos = pObject->GetGlobalPosition();
//      const ezQuat rot = pObject->GetGlobalRotation();
//
//      PxTransform t = ezPxConversionUtils::ToTransform(pos, rot);
//      pActor->setKinematicTarget(t);
//    }
//  }
//}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPxTriggerComponent, 1)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//  }
//  EZ_END_PROPERTIES
//}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxTriggerComponent::ezPxTriggerComponent()
  : m_UserData(this)
{
  m_pActor = nullptr;
}

void ezPxTriggerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  //auto& s = stream.GetStream();
}

void ezPxTriggerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  //auto& s = stream.GetStream();
}

void ezPxTriggerComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  PxTransform t = ezPxConversionUtils::ToTransform(globalTransform);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidDynamic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  m_pActor->userData = &m_UserData;

  AddShapesFromObject(GetOwner(), m_pActor, globalTransform);

  const ezUInt32 uiNumShapes = m_pActor->getNbShapes();
  if (uiNumShapes == 0)
  {
    m_pActor->release();
    m_pActor = nullptr;

    ezLog::Error("Trigger '{0}' does not have any shape components. Actor will be removed.", GetOwner()->GetName());
    return;
  }

  // set the trigger flag on all attached shapes
  {
    ezHybridArray<PxShape*, 16> shapes;
    shapes.SetCountUninitialized(uiNumShapes);
    m_pActor->getShapes(shapes.GetData(), uiNumShapes);

    for (ezUInt32 i = 0; i < uiNumShapes; ++i)
    {
      shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
      shapes[i]->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
  }

  // not really sure about these for triggers
  m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
  m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxTriggerComponent::Deinitialize()
{
  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->release();
    m_pActor = nullptr;
  }
}

