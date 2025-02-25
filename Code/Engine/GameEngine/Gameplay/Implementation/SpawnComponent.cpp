#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/SpawnComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSpawnComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
    EZ_ACCESSOR_PROPERTY("AttachAsChild", GetAttachAsChild, SetAttachAsChild),
    EZ_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    EZ_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    EZ_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1.0))),
    EZ_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::MakeFromDegree(179.0))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColorScheme::LightUI(ezColorScheme::Lime)),
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "Deviation", 0.5f, nullptr, ezColorScheme::LightUI(ezColorScheme::Lime)),
    new ezConeAngleManipulatorAttribute("Deviation", 0.5f),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(CanTriggerManualSpawn),
    EZ_SCRIPT_FUNCTION_PROPERTY(TriggerManualSpawn, In, "IgnoreSpawnDelay", In, "LocalOffset"),
    EZ_SCRIPT_FUNCTION_PROPERTY(ScheduleSpawn),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSpawnComponent::ezSpawnComponent() = default;
ezSpawnComponent::~ezSpawnComponent() = default;

void ezSpawnComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnAtStart))
  {
    ScheduleSpawn();
  }
}


void ezSpawnComponent::OnDeactivated()
{
  m_SpawnFlags.Remove(ezSpawnComponentFlags::SpawnInFlight);

  SUPER::OnDeactivated();
}

bool ezSpawnComponent::SpawnOnce(const ezVec3& vLocalOffset)
{
  if (m_hPrefab.IsValid())
  {
    ezTransform tLocalSpawn;
    tLocalSpawn.SetIdentity();
    tLocalSpawn.m_vPosition = vLocalOffset;

    if (m_MaxDeviation.GetRadian() > 0)
    {
      const ezVec3 vTiltAxis = ezVec3(0, 1, 0);
      const ezVec3 vTurnAxis = ezVec3(1, 0, 0);

      const ezAngle tiltAngle = ezAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, (double)m_MaxDeviation.GetRadian()));
      const ezAngle turnAngle = ezAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, ezMath::Pi<double>() * 2.0));

      ezQuat qTilt, qTurn, qDeviate;
      qTilt = ezQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);
      qTurn = ezQuat::MakeFromAxisAndAngle(vTurnAxis, turnAngle);
      qDeviate = qTurn * qTilt;

      tLocalSpawn.m_qRotation = qDeviate;
    }

    DoSpawn(tLocalSpawn);

    return true;
  }

  return false;
}


void ezSpawnComponent::DoSpawn(const ezTransform& tLocalSpawn)
{
  ezResourceLock<ezPrefabResource> pResource(m_hPrefab, ezResourceAcquireMode::AllowLoadingFallback);

  ezPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::AttachAsChild))
  {
    options.m_hParent = GetOwner()->GetHandle();

    pResource->InstantiatePrefab(*GetWorld(), tLocalSpawn, options, &m_Parameters);
  }
  else
  {
    ezTransform tGlobalSpawn;
    tGlobalSpawn = ezTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), tLocalSpawn);
    tGlobalSpawn.m_vScale.Set(1);

    pResource->InstantiatePrefab(*GetWorld(), tGlobalSpawn, options, &m_Parameters);
  }
}

void ezSpawnComponent::ScheduleSpawn()
{
  if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnInFlight))
    return;

  ezMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("scheduled_spawn");

  m_SpawnFlags.Add(ezSpawnComponentFlags::SpawnInFlight);

  ezWorld* pWorld = GetWorld();

  const ezTime tKill = ezTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleMinMax(m_MinDelay.GetSeconds(), m_MinDelay.GetSeconds() + m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);
}

void ezSpawnComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_SpawnFlags.GetValue();
  s << m_hPrefab;

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_MaxDeviation;
  s << m_LastManualSpawn;

  ezPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void ezSpawnComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  ezSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_SpawnFlags.SetValue(flags);

  s >> m_hPrefab;

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_MaxDeviation;
  s >> m_LastManualSpawn;

  if (uiVersion >= 3)
  {
    ezPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

bool ezSpawnComponent::CanTriggerManualSpawn() const
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  return tNow - m_LastManualSpawn >= m_MinDelay;
}

bool ezSpawnComponent::TriggerManualSpawn(bool bIgnoreSpawnDelay /*= false*/, const ezVec3& vLocalOffset /*= ezVec3::MakeZero()*/)
{
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (bIgnoreSpawnDelay == false && tNow - m_LastManualSpawn < m_MinDelay)
    return false;

  m_LastManualSpawn = tNow;
  return SpawnOnce(vLocalOffset);
}

bool ezSpawnComponent::GetSpawnAtStart() const
{
  return m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnAtStart);
}

void ezSpawnComponent::SetSpawnAtStart(bool b)
{
  m_SpawnFlags.AddOrRemove(ezSpawnComponentFlags::SpawnAtStart, b);
}

bool ezSpawnComponent::GetSpawnContinuously() const
{
  return m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnContinuously);
}

void ezSpawnComponent::SetSpawnContinuously(bool b)
{
  m_SpawnFlags.AddOrRemove(ezSpawnComponentFlags::SpawnContinuously, b);
}

bool ezSpawnComponent::GetAttachAsChild() const
{
  return m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::AttachAsChild);
}

void ezSpawnComponent::SetAttachAsChild(bool b)
{
  m_SpawnFlags.AddOrRemove(ezSpawnComponentFlags::AttachAsChild, b);
}

void ezSpawnComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == ezTempHashedString("scheduled_spawn"))
  {
    m_SpawnFlags.Remove(ezSpawnComponentFlags::SpawnInFlight);

    SpawnOnce(ezVec3::MakeZero());

    // do it all again
    if (m_SpawnFlags.IsAnySet(ezSpawnComponentFlags::SpawnContinuously))
    {
      ScheduleSpawn();
    }
  }
  else if (msg.m_sMessage == ezTempHashedString("spawn"))
  {
    TriggerManualSpawn();
  }
}

const ezRangeView<const char*, ezUInt32> ezSpawnComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void ezSpawnComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void ezSpawnComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(ezTempHashedString(szKey));
}

bool ezSpawnComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSpawnComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSpawnComponentPatch_1_2()
    : ezGraphPatch("ezSpawnComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Attach as Child", "AttachAsChild");
    pNode->RenameProperty("Spawn at Start", "SpawnAtStart");
    pNode->RenameProperty("Spawn Continuously", "SpawnContinuously");
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
  }
};

ezSpawnComponentPatch_1_2 g_ezSpawnComponentPatch_1_2;



EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnComponent);
