#include <PCH.h>
#include <GameEngine/Components/TimedDeathComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Core/Messages/TriggerMessage.h>

EZ_BEGIN_COMPONENT_TYPE(ezTimedDeathComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
    EZ_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_ACCESSOR_PROPERTY("TimeoutPrefab", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
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
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

void ezTimedDeathComponent::OnSimulationStarted()
{
  ezMsgComponentInternalTrigger msg;
  msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("Suicide");

  ezWorld* pWorld = GetWorld();

  const ezTime tKill = ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  PostMessage(msg, ezObjectMsgQueueType::NextFrame, tKill);

  // make sure the prefab is available when the component dies
  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceManager::PreloadResource(m_hTimeoutPrefab, tKill);
  }
}

void ezTimedDeathComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_uiUsageStringHash != ezTempHashedString::ComputeHash("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hTimeoutPrefab);

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), ezGameObjectHandle(), nullptr, &GetOwner()->GetTeamID(), nullptr);
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezTimedDeathComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezTimedDeathComponentPatch_1_2()
    : ezGraphPatch("ezTimedDeathComponent", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
  }
};

ezTimedDeathComponentPatch_1_2 g_ezTimedDeathComponentPatch_1_2;



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_TimedDeathComponent);

