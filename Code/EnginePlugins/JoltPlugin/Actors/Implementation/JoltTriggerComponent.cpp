#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

ezJoltTriggerComponentManager::ezJoltTriggerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltTriggerComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltTriggerComponentManager::~ezJoltTriggerComponentManager() = default;

void ezJoltTriggerComponentManager::UpdateMovingTriggers()
{
  EZ_PROFILE_SCOPE("MovingTriggers");

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto& bodyInterface = pModule->GetJoltSystem()->GetBodyInterface();

  for (auto pTrigger : m_MovingTriggers)
  {
    JPH::BodyID bodyId(pTrigger->m_uiJoltBodyID);

    ezSimdTransform trans = pTrigger->GetOwner()->GetGlobalTransformSimd();

    bodyInterface.SetPositionAndRotation(bodyId, ezJoltConversionUtils::ToVec3(trans.m_Position), ezJoltConversionUtils::ToQuat(trans.m_Rotation), JPH::EActivation::Activate);
  }
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

void ezJoltTriggerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_sTriggerMessage;
}

void ezJoltTriggerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sTriggerMessage;
}

void ezJoltTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, nullptr).Failed())
  {
    ezLog::Error("Jolt trigger actor component has no valid shape.");
    return;
  }

  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  bodyCfg.mIsSensor = true;
  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Trigger);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  EZ_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, false);

  if (GetOwner()->IsDynamic())
  {
    ezJoltTriggerComponentManager* pManager = static_cast<ezJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Insert(this);
  }
}

void ezJoltTriggerComponent::OnDeactivated()
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezJoltContactListener* pContactListener = pModule->GetContactListener();
  pContactListener->RemoveTrigger(this);

  if (GetOwner()->IsDynamic())
  {
    ezJoltTriggerComponentManager* pManager = static_cast<ezJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Remove(this);
  }

  SUPER::OnDeactivated();
}

void ezJoltTriggerComponent::PostTriggerMessage(const ezGameObjectHandle& hOtherObject, ezTriggerState::Enum triggerState) const
{
  ezMsgTriggerTriggered msg;

  msg.m_TriggerState = triggerState;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = hOtherObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), ezTime::MakeZero(), ezObjectMsgQueueType::PostTransform);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltTriggerComponent);
