#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Messages/DamageMessage.h>

ezAiPerceptionGenPOI::ezAiPerceptionGenPOI() = default;
ezAiPerceptionGenPOI::~ezAiPerceptionGenPOI() = default;

void ezAiPerceptionGenPOI::UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  const ezAiSensorSpatial* pSensorSee = static_cast<const ezAiSensorSpatial*>(ref_SensorManager.GetSensor("Sensor_See"));
  if (pSensorSee == nullptr)
    return;

  ezWorld* pWorld = owner.GetWorld();

  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
  pSensorSee->RetrieveSensations(owner, detectedObjects);

  for (ezGameObjectHandle hObj : detectedObjects)
  {
    ezGameObject* pObj = nullptr;
    if (pWorld->TryGetObject(hObj, pObj))
    {
      auto& g = m_Perceptions.ExpandAndGetRef();
      g.m_vGlobalPosition = pObj->GetGlobalPosition();
    }
  }
}

bool ezAiPerceptionGenPOI::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void ezAiPerceptionGenPOI::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void ezAiPerceptionGenPOI::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  ref_SensorManager.FlagAsNeeded("Sensor_See");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenWander::ezAiPerceptionGenWander() = default;
ezAiPerceptionGenWander::~ezAiPerceptionGenWander() = default;

void ezAiPerceptionGenWander::UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  ezWorld* pWorld = owner.GetWorld();
  const ezVec3 c = owner.GetGlobalPosition();
  const ezVec3 dir = 3.0f * owner.GetGlobalDirForwards();
  const ezVec3 right = 5.0f * owner.GetGlobalDirRight();

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir - right;
  }
}

bool ezAiPerceptionGenWander::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void ezAiPerceptionGenWander::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void ezAiPerceptionGenWander::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenCheckpoint::ezAiPerceptionGenCheckpoint() = default;
ezAiPerceptionGenCheckpoint::~ezAiPerceptionGenCheckpoint() = default;

void ezAiPerceptionGenCheckpoint::UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  ezWorld* pWorld = owner.GetWorld();
  const ezVec3 c = owner.GetGlobalPosition();
  const ezVec3 dir = 3.0f * owner.GetGlobalDirForwards();
  const ezVec3 right = 5.0f * owner.GetGlobalDirRight();

  if (m_SpatialCategory == ezInvalidSpatialDataCategory)
  {
    m_SpatialCategory = ezSpatialData::RegisterCategory("Checkpoint", ezSpatialData::Flags::None);
  }

  ezHybridArray<ezGameObject*, 32> checkpoints;

  ezSpatialSystem::QueryParams param;
  param.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();
  pWorld->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(c, 100.0f), param, checkpoints);

  for (ezUInt32 i = 0; i < checkpoints.GetCount(); ++i)
  {
    auto& p = m_Perceptions.ExpandAndGetRef();
    p.m_vGlobalPosition = checkpoints[i]->GetGlobalPosition();
  }
}

bool ezAiPerceptionGenCheckpoint::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void ezAiPerceptionGenCheckpoint::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void ezAiPerceptionGenCheckpoint::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenTarget::ezAiPerceptionGenTarget() = default;
ezAiPerceptionGenTarget::~ezAiPerceptionGenTarget() = default;

void ezAiPerceptionGenTarget::UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager)
{
  ezWorld* pWorld = owner.GetWorld();
  const ezTime tNow = pWorld->GetClock().GetAccumulatedTime();

  ezGameObject* pVisible = nullptr;

  for (const ezString& sName : m_SensorNames)
  {
    const ezAiSensorSpatial* pSensor = static_cast<const ezAiSensorSpatial*>(ref_SensorManager.GetSensor(sName));
    if (pSensor == nullptr)
      continue;

    ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
    pSensor->RetrieveSensations(owner, detectedObjects);

    for (ezGameObjectHandle hObj : detectedObjects)
    {
      if (pWorld->TryGetObject(hObj, pVisible))
        break;
    }

    if (pVisible)
      break;
  }

  if (pVisible)
  {
    m_bHasTarget = true;
    m_bDamageReceived = false;
    m_Target.m_hTarget = pVisible->GetHandle();
    m_Target.m_vGlobalPosition = pVisible->GetGlobalPosition();
    m_Target.m_bVisible = true;
    m_Target.m_bAlerted = false;
    m_Target.m_LastSeen = tNow;
  }
  else
  {
    m_Target.m_bVisible = false;

    if (m_bDamageReceived)
    {
      m_bDamageReceived = false;

      ezGameObject* pSource = nullptr;
      if (!m_sDamageSourceGlobalKey.IsEmpty() && pWorld->TryGetObjectWithGlobalKey(ezTempHashedString(m_sDamageSourceGlobalKey), pSource))
      {
        m_bHasTarget = true;
        m_Target.m_hTarget = pSource->GetHandle();
        m_Target.m_vGlobalPosition = pSource->GetGlobalPosition();
        m_Target.m_bAlerted = true;
        m_Target.m_LastSeen = tNow;
      }
    }

    if (m_bHasTarget && tNow - m_Target.m_LastSeen > m_MemoryDuration)
    {
      m_bHasTarget = false;
    }
  }

  if (m_bHasTarget)
  {
    m_Target.m_fDistance = (m_Target.m_vGlobalPosition - owner.GetGlobalPosition()).GetLength();
  }
}

bool ezAiPerceptionGenTarget::HasPerceptions() const
{
  return m_bHasTarget;
}

void ezAiPerceptionGenTarget::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  if (m_bHasTarget)
  {
    out_Perceptions.PushBack(&m_Target);
  }
}

void ezAiPerceptionGenTarget::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  for (const ezString& sName : m_SensorNames)
  {
    ref_SensorManager.FlagAsNeeded(sName);
  }
}

void ezAiPerceptionGenTarget::HandleMessage(ezGameObject& owner, const ezMessage& msg)
{
  if (msg.GetId() == ezMsgDamage::GetTypeMsgId())
  {
    m_bDamageReceived = true;
  }
}
