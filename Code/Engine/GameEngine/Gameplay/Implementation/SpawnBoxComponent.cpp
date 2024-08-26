#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/SpawnBoxComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSpawnBoxComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(2.0f, 2.0f, 0.25f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    EZ_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    EZ_MEMBER_PROPERTY("MinSpawnCount", m_uiMinSpawnCount)->AddAttributes(new ezDefaultValueAttribute(10)),
    EZ_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Duration", m_SpawnDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(5))),
    EZ_MEMBER_PROPERTY("MaxRotationZ", m_MaxRotationZ),
    EZ_MEMBER_PROPERTY("MaxTiltZ", m_MaxTiltZ),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new ezBoxVisualizerAttribute("HalfExtents", 2.0f),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColorScheme::LightUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(StartSpawning),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

void ezSpawnBoxComponent::SetHalfExtents(const ezVec3& value)
{
  m_vHalfExtents = value.CompMax(ezVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool ezSpawnBoxComponent::GetSpawnAtStart() const
{
  return m_Flags.IsAnySet(ezSpawnBoxComponentFlags::SpawnAtStart);
}

void ezSpawnBoxComponent::SetSpawnAtStart(bool b)
{
  m_Flags.AddOrRemove(ezSpawnBoxComponentFlags::SpawnAtStart, b);
}

bool ezSpawnBoxComponent::GetSpawnContinuously() const
{
  return m_Flags.IsAnySet(ezSpawnBoxComponentFlags::SpawnContinuously);
}

void ezSpawnBoxComponent::SetSpawnContinuously(bool b)
{
  m_Flags.AddOrRemove(ezSpawnBoxComponentFlags::SpawnContinuously, b);
}

void ezSpawnBoxComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
  s << m_hPrefab;
  s << m_Flags;
  s << m_SpawnDuration;
  s << m_uiMinSpawnCount;
  s << m_uiSpawnCountRange;
  s << m_MaxRotationZ;
  s << m_MaxTiltZ;
}

void ezSpawnBoxComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
  s >> m_hPrefab;
  s >> m_Flags;
  s >> m_SpawnDuration;
  s >> m_uiMinSpawnCount;
  s >> m_uiSpawnCountRange;
  s >> m_MaxRotationZ;
  s >> m_MaxTiltZ;
}

void ezSpawnBoxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetSpawnAtStart())
  {
    StartSpawning();
  }
}

void ezSpawnBoxComponent::StartSpawning()
{
  InternalStartSpawning(true);
}

void ezSpawnBoxComponent::InternalStartSpawning(bool bFirstTime)
{

  m_uiSpawned = 0;
  m_uiTotalToSpawn = m_uiMinSpawnCount;
  m_StartTime = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_uiSpawnCountRange > 0)
  {
    m_uiTotalToSpawn = GetWorld()->GetRandomNumberGenerator().IntInRange(m_uiMinSpawnCount, m_uiSpawnCountRange);
  }

  if (m_uiTotalToSpawn == 0)
    return;

  if (m_SpawnDuration.IsZeroOrNegative())
  {
    Spawn(m_uiTotalToSpawn);
  }
  else
  {
    if (bFirstTime)
    {
      // this guarantees that next time OnTriggered() is called, one object gets spawned right away
      m_StartTime -= m_SpawnDuration / m_uiTotalToSpawn;
    }

    ezMsgComponentInternalTrigger msg;
    PostMessage(msg, ezTime::MakeZero());
  }
}

void ezSpawnBoxComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  const ezTime tActive = tNow - m_StartTime;
  const ezTime tEnd = m_StartTime + m_SpawnDuration;

  if (tNow >= tEnd)
  {
    if (m_uiSpawned < m_uiTotalToSpawn)
    {
      Spawn(m_uiTotalToSpawn - m_uiSpawned);
    }

    if (GetSpawnContinuously())
    {
      InternalStartSpawning(false);
    }

    return;
  }

  const auto uiTargetSpawnCount = ezMath::Clamp<ezUInt16>(static_cast<ezUInt16>(((tActive.GetSeconds() / m_SpawnDuration.GetSeconds()) * m_uiTotalToSpawn)), 0, m_uiTotalToSpawn);

  if (m_uiSpawned < uiTargetSpawnCount)
  {
    Spawn(uiTargetSpawnCount - m_uiSpawned);
  }

  if (m_uiSpawned < m_uiTotalToSpawn)
  {
    // remaining time divided equally for the remaining spawns
    // this is to prevent a lot of unnecessary message sending at low spawn counts
    ezTime tDelay = (tEnd - tNow) / (m_uiTotalToSpawn - m_uiSpawned);

    // prevent unnecessary high number of updates, rather spawn multiple objects within one frame
    tDelay = ezMath::Max(tDelay, ezTime::MakeFromMilliseconds(40)); // max 25 Hz

    ezMsgComponentInternalTrigger msg;
    PostMessage(msg, tDelay);
  }
  else if (GetSpawnContinuously())
  {
    InternalStartSpawning(false);
  }
}

void ezSpawnBoxComponent::Spawn(ezUInt32 uiCount)
{
  if (uiCount == 0)
    return;

  m_uiSpawned += uiCount;

  if (!m_hPrefab.IsValid())
    return;

  ezResourceLock<ezPrefabResource> pResource(m_hPrefab, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pResource.GetAcquireResult() == ezResourceAcquireResult::None)
    return;

  ezPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  ezRandom& rnd = GetWorld()->GetRandomNumberGenerator();
  const ezTransform tOwner = GetOwner()->GetGlobalTransform();

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezTransform tLocal = ezTransform::MakeIdentity();
    tLocal.m_vPosition.x = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.x, m_vHalfExtents.x));
    tLocal.m_vPosition.y = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.y, m_vHalfExtents.y));
    tLocal.m_vPosition.z = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.z, m_vHalfExtents.z));

    if (m_MaxRotationZ.GetRadian() > 0)
    {
      const ezAngle rotationAngle = ezAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(-m_MaxRotationZ.GetRadian(), +m_MaxRotationZ.GetRadian()));
      const ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), rotationAngle);

      tLocal.m_qRotation = qRot;
    }

    if (m_MaxTiltZ.GetRadian() > 0)
    {
      const ezAngle tiltTurnAngle = ezAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, ezMath::Pi<double>() * 2.0));
      const ezQuat qTiltTurn = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), tiltTurnAngle);

      const ezVec3 vTiltAxis = qTiltTurn * ezVec3(1, 0, 0);

      const ezAngle tiltAngle = ezAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, (double)m_MaxTiltZ.GetRadian()));
      const ezQuat qTilt = ezQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);

      tLocal.m_qRotation = tLocal.m_qRotation * qTilt;
    }

    const ezTransform tGlobal = ezTransform::MakeGlobalTransform(tOwner, tLocal);

    pResource->InstantiatePrefab(*GetWorld(), tGlobal, options);
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnBoxComponent);
