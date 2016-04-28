#include <GameUtils/PCH.h>
#include <GameUtils/Components/TimedDeathComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GameUtils/Prefabs/PrefabResource.h>

EZ_BEGIN_COMPONENT_TYPE(ezTimedDeathComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Min Delay", m_MinDelay)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
    EZ_MEMBER_PROPERTY("Delay Range", m_DelayRange)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Timeout Prefab", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezTriggerMessage, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTimedDeathComponent::ezTimedDeathComponent()
{
  m_MinDelay = ezTime::Seconds(1.0);
  m_DelayRange = ezTime::Seconds(0.0);
}

void ezTimedDeathComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_hTimeoutPrefab;
}

void ezTimedDeathComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

ezComponent::Initialization ezTimedDeathComponent::Initialize()
{
  ezTriggerMessage msg;
  msg.m_hTargetComponent = GetHandle();
  msg.m_UsageStringHash = ezTempHashedString("Suicide").GetHash();

  ezWorld* pWorld = GetWorld();

  const ezTime tKill = ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  pWorld->PostMessage(GetOwner()->GetHandle(), msg, ezObjectMsgQueueType::NextFrame, tKill, ezObjectMsgRouting::ToComponents);

  // make sure the prefab is available when the component dies
  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceManager::PreloadResource(m_hTimeoutPrefab, tKill);
  }

  return ezComponent::Initialization::Done;
}

void ezTimedDeathComponent::OnTriggered(ezTriggerMessage& msg)
{
  // mass suicide is theoretically possible by sending this message without a specific target
  if (!msg.m_hTargetComponent.IsInvalidated() && msg.m_hTargetComponent != GetHandle())
    return;

  if (msg.m_UsageStringHash != ezTempHashedString("Suicide").GetHash())
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hTimeoutPrefab);

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform());
  }
  
  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void ezTimedDeathComponent::SetTimeoutPrefab(const char* szPrefab)
{
  ezPrefabResourceHandle hPrefab;

  if (!ezStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szPrefab);
  }

  m_hTimeoutPrefab = hPrefab;
}


const char* ezTimedDeathComponent::GetTimeoutPrefab() const
{
  if (!m_hTimeoutPrefab.IsValid())
    return "";

  return m_hTimeoutPrefab.GetResourceID();
}


