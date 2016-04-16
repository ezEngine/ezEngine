#include <GameUtils/PCH.h>
#include <GameUtils/Components/SpawnComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Components/InputComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ezSpawnComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_ACCESSOR_PROPERTY("Attach as Child", GetAttachAsChild, SetAttachAsChild),
    EZ_ACCESSOR_PROPERTY("Spawn at Start", GetSpawnAtStart, SetSpawnAtStart),
    EZ_ACCESSOR_PROPERTY("Spawn Continuously", GetSpawnContinuously, SetSpawnContinuously),
    EZ_MEMBER_PROPERTY("Min Delay", m_MinDelay)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
    EZ_MEMBER_PROPERTY("Delay Range", m_DelayRange)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::Degree(179.0))),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::YellowGreen),
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "Deviation", 0.5f, nullptr, nullptr, ezColor::GreenYellow),
    new ezConeManipulatorAttribute("Deviation", 0.5f),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezInputComponentMessage, InputComponentMessageHandler),
    EZ_MESSAGE_HANDLER(ezComponentTriggerMessage, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpawnComponent::ezSpawnComponent()
{
}

ezComponent::Initialization ezSpawnComponent::Initialize()
{
  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnAtStart))
  {
    m_SpawnFlags.Remove(ezSpawnComponentFlags::SpawnAtStart);

    ScheduleSpawn();
  }

  return ezComponent::Initialization::Done;
}


bool ezSpawnComponent::SpawnOnce()
{
  if (m_hPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pResource(m_hPrefab);

    ezTransform tLocalSpawn;
    tLocalSpawn.SetIdentity();

    if (m_MaxDeviation.GetRadian() > 0)
    {
      const ezVec3 vTiltAxis = ezVec3(0, 1, 0);
      const ezVec3 vTurnAxis = ezVec3(1, 0, 0);

      const ezAngle tiltAngle = ezAngle::Radian((float) GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, (double)m_MaxDeviation.GetRadian()));
      const ezAngle turnAngle = ezAngle::Radian((float) GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, ezMath::BasicType<double>::Pi() * 2.0));

      ezQuat qTilt, qTurn, qDeviate;
      qTilt.SetFromAxisAndAngle(vTiltAxis, tiltAngle);
      qTurn.SetFromAxisAndAngle(vTurnAxis, turnAngle);
      qDeviate = qTurn * qTilt;

      tLocalSpawn.m_Rotation = qDeviate.GetAsMat3();
    }

    if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::AttachAsChild))
    {
      pResource->InstantiatePrefab(*GetWorld(), tLocalSpawn, GetOwner()->GetHandle());
    }
    else
    {
      ezTransform tGlobalSpawn;
      tGlobalSpawn.SetGlobalTransform(GetOwner()->GetGlobalTransform(), tLocalSpawn);

      pResource->InstantiatePrefab(*GetWorld(), tGlobalSpawn, ezGameObjectHandle());
    }
    return true;
  }

  return false;
}


void ezSpawnComponent::ScheduleSpawn()
{
  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnInFlight))
    return;

  ezComponentTriggerMessage msg;
  msg.m_hTargetComponent = GetHandle();

  m_SpawnFlags.Add(ezSpawnComponentFlags::SpawnInFlight);

  ezWorld* pWorld = GetWorld();

  const ezTime tKill = ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  pWorld->PostMessage(GetOwner()->GetHandle(), msg, ezObjectMsgQueueType::NextFrame, tKill, ezObjectMsgRouting::ToComponents);
}

void ezSpawnComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_SpawnFlags.GetValue();
  s << m_hPrefab;

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_MaxDeviation;
  s << m_LastManualSpawn;
}

void ezSpawnComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  ezSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_SpawnFlags.SetValue(flags);

  s >> m_hPrefab;

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_MaxDeviation; 
  s >> m_LastManualSpawn;
}


bool ezSpawnComponent::TriggerManualSpawn()
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (tNow - m_LastManualSpawn < m_MinDelay)
    return false;

  m_LastManualSpawn = tNow;
  return SpawnOnce();
}

void ezSpawnComponent::SetPrefabFile(const char* szFile)
{
  ezPrefabResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(szFile);
  }

  SetPrefab(hResource);
}

const char* ezSpawnComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

void ezSpawnComponent::SetPrefab(const ezPrefabResourceHandle& hPrefab)
{
  m_hPrefab = hPrefab;
}


void ezSpawnComponent::OnTriggered(ezComponentTriggerMessage& msg)
{
  if (msg.m_hTargetComponent != GetHandle())
    return;

  m_SpawnFlags.Remove(ezSpawnComponentFlags::SpawnInFlight);

  SpawnOnce();

  // do it all again
  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnContinuously))
  {
    ScheduleSpawn();
  }
}

void ezSpawnComponent::InputComponentMessageHandler(ezInputComponentMessage& msg)
{
  float f = msg.m_fValue;

  if (ezStringUtils::IsEqual(msg.m_szAction, "spawn"))
  {
    TriggerManualSpawn();
  }
}

