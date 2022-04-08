#include <JoltPlugin/JoltPluginPCH.h>

#if 0

#  include <Core/WorldSerializer/WorldReader.h>
#  include <Core/WorldSerializer/WorldWriter.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <JoltPlugin/Actors/JoltTriggerComponent.h>
#  include <JoltPlugin/Shapes/JoltShapeComponent.h>
#  include <JoltPlugin/System/JoltWorldModule.h>
#  include <JoltPlugin/Utilities/JoltConversionUtils.h>

ezJoltTriggerComponentManager::ezJoltTriggerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltTriggerComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltTriggerComponentManager::~ezJoltTriggerComponentManager() {}

void ezJoltTriggerComponentManager::UpdateKinematicActors()
{
  //EZ_PROFILE_SCOPE("KinematicActors");

  //for (auto pKinematicActorComponent : m_KinematicActorComponents)
  //{
  //  if (PxRigidDynamic* pActor = pKinematicActorComponent->GetActor())
  //  {
  //    ezGameObject* pObject = pKinematicActorComponent->GetOwner();

  //    pObject->UpdateGlobalTransform();

  //    const ezVec3 pos = pObject->GetGlobalPosition();
  //    const ezQuat rot = pObject->GetGlobalRotation();

  //    PxTransform t = ezJoltConversionUtils::ToTransform(pos, rot);
  //    pActor->setKinematicTarget(t);
  //  }
  //}
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltTriggerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_TriggerEventSender)
  }
  EZ_END_MESSAGESENDERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltTriggerComponent::ezJoltTriggerComponent() = default;
ezJoltTriggerComponent::~ezJoltTriggerComponent() = default;

void ezJoltTriggerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  bool m_bKinematic = false;
  s << m_bKinematic;
  s << m_sTriggerMessage;
}

void ezJoltTriggerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  bool m_bKinematic = false;
  s >> m_bKinematic;
  s >> m_sTriggerMessage;
}

void ezJoltTriggerComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  //const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  //JoltTransform t = ezJoltConversionUtils::ToTransform(globalTransform);
  //m_pActor = ezJolt::GetSingleton()->GetJoltAPI()->createRigidDynamic(t);
  //EZ_ASSERT_DEBUG(m_pActor != nullptr, "Jolt actor creation failed");

  //ezJoltUserData* pUserData = nullptr;
  //m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  //pUserData->Init(this);
  //m_pActor->userData = pUserData;

  //// Jolt does not get any scale value, so to correctly position child objects
  //// we have to pretend that this parent object applies no scale on its children
  //ezSimdTransform globalTransformNoScale = globalTransform;
  //globalTransformNoScale.m_Scale.Set(1.0f);
  //AddShapesFromObject(GetOwner(), m_pActor, globalTransformNoScale);

  //const ezUInt32 uiNumShapes = m_pActor->getNbShapes();
  //if (uiNumShapes == 0)
  //{
  //  m_pActor->release();
  //  m_pActor = nullptr;

  //  ezLog::Error("Trigger '{0}' does not have any shape components. Actor will be removed.", GetOwner()->GetName());
  //  return;
  //}

  //// set the trigger flag on all attached shapes
  //{
  //  ezHybridArray<PxShape*, 16> shapes;
  //  shapes.SetCountUninitialized(uiNumShapes);
  //  m_pActor->getShapes(shapes.GetData(), uiNumShapes);

  //  for (ezUInt32 i = 0; i < uiNumShapes; ++i)
  //  {
  //    shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
  //    shapes[i]->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
  //  }
  //}

  //m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
  //m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true); // triggers are never dynamic

  //{
  //  EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
  //  pModule->GetPxScene()->addActor(*m_pActor);
  //}

  //if (GetOwner()->IsDynamic())
  //{
  //  GetWorld()->GetOrCreateComponentManager<ezJoltTriggerComponentManager>()->m_KinematicActorComponents.PushBack(this);
  //}
}

void ezJoltTriggerComponent::OnDeactivated()
{
  //if (m_pActor != nullptr)
  //{
  //  EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

  //  m_pActor->release();
  //  m_pActor = nullptr;
  //}

  SUPER::OnDeactivated();
}

void ezJoltTriggerComponent::PostTriggerMessage(const ezComponent* pOtherComponent, ezTriggerState::Enum triggerState) const
{
  ezMsgTriggerTriggered msg;

  msg.m_TriggerState = triggerState;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = pOtherComponent->GetOwner()->GetHandle();

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
}

#endif
