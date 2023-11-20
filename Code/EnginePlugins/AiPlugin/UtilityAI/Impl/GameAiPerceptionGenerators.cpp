#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiMarkerComponent.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionPOI, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionWander, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionCheckpoint, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionTarget, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Cast, In, "Perception"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTarget),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetDistance),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsVisible),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetLastSeen),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionHint, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Cast, In, "Perception"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetMarker),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetSource),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetSourcePosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetDistance),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPriority),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenPOI, 1, ezRTTIDefaultAllocator<ezAiPerceptionGenPOI>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAiPerceptionTypeAttribute("ezAiPerceptionPOI"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Sensors", m_Sensors)->AddAttributes(new ezAiInputPinAttribute("ezAiSensorSpatial"), new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenWander, 1, ezRTTIDefaultAllocator<ezAiPerceptionGenWander>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAiPerceptionTypeAttribute("ezAiPerceptionWander"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenCheckpoint, 1, ezRTTIDefaultAllocator<ezAiPerceptionGenCheckpoint>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAiPerceptionTypeAttribute("ezAiPerceptionCheckpoint"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenTarget, 1, ezRTTIDefaultAllocator<ezAiPerceptionGenTarget>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAiPerceptionTypeAttribute("ezAiPerceptionTarget"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Sensors", m_Sensors)->AddAttributes(new ezAiInputPinAttribute("ezAiSensorSpatial"), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("MemoryDuration", m_MemoryDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(30))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenHint, 1, ezRTTIDefaultAllocator<ezAiPerceptionGenHint>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAiPerceptionTypeAttribute("ezAiPerceptionHint"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Sensors", m_Sensors)->AddAttributes(new ezAiInputPinAttribute("ezAiSensorSpatial"), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("DirectSensors", m_DirectSensors)->AddAttributes(new ezAiInputPinAttribute("ezAiSensorSpatial"), new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// static
ezAiPerceptionTarget* ezAiPerceptionTarget::Cast(const ezAiPerception* pPerception)
{
  return const_cast<ezAiPerceptionTarget*>(ezDynamicCast<const ezAiPerceptionTarget*>(pPerception));
}

// static
void ezAiPerceptionTarget::GetDebugDesc(ezStringBuilder& out_sText) const
{
  if (m_bVisible)
    out_sText.SetFormat("visible, dist {}", ezArgF(m_fDistance, 1));
  else
    out_sText.SetFormat("not visible, last seen {}", ezArgF(m_LastSeen.AsFloatInSeconds(), 2));
}

ezAiPerceptionHint* ezAiPerceptionHint::Cast(const ezAiPerception* pPerception)
{
  return const_cast<ezAiPerceptionHint*>(ezDynamicCast<const ezAiPerceptionHint*>(pPerception));
}

void ezAiPerceptionHint::GetDebugDesc(ezStringBuilder& out_sText) const
{
  out_sText.SetFormat("priority {}, dist {}", ezArgF(m_fPriority, 2), ezArgF(m_fDistance, 1));
}

ezAiPerceptionGenPOI::ezAiPerceptionGenPOI() = default;
ezAiPerceptionGenPOI::~ezAiPerceptionGenPOI() = default;

void ezAiPerceptionGenPOI::UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& perceptions = static_cast<InstanceData*>(pInstanceData)->m_Perceptions;
  perceptions.Clear();

  ezWorld* pWorld = ctx.m_Owner.GetWorld();

  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    const void* pSensorData = nullptr;
    const ezAiSensorSpatial* pSensor = static_cast<const ezAiSensorSpatial*>(ctx.m_SensorManager.GetSensor(uiSensorIndex, pSensorData));
    pSensor->RetrieveSensations(ctx, pSensorData, detectedObjects);
  }

  for (ezGameObjectHandle hObj : detectedObjects)
  {
    ezGameObject* pObj = nullptr;
    if (pWorld->TryGetObject(hObj, pObj))
    {
      auto& g = perceptions.ExpandAndGetRef();
      g.m_vGlobalPosition = pObj->GetGlobalPosition();
    }
  }
}

void ezAiPerceptionGenPOI::FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const
{
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    ref_SensorManager.FlagAsNeeded(uiSensorIndex);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenWander::ezAiPerceptionGenWander() = default;
ezAiPerceptionGenWander::~ezAiPerceptionGenWander() = default;

void ezAiPerceptionGenWander::UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& perceptions = static_cast<InstanceData*>(pInstanceData)->m_Perceptions;
  perceptions.Clear();

  const ezVec3 c = ctx.m_Owner.GetGlobalPosition();
  const ezVec3 dir = 3.0f * ctx.m_Owner.GetGlobalDirForwards();
  const ezVec3 right = 5.0f * ctx.m_Owner.GetGlobalDirRight();

  perceptions.ExpandAndGetRef().m_vGlobalPosition = c + dir;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c + right;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c - right;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c + dir + right;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c + dir - right;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c - dir + right;
  perceptions.ExpandAndGetRef().m_vGlobalPosition = c - dir - right;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenCheckpoint::ezAiPerceptionGenCheckpoint()
{
  m_SpatialCategory = ezSpatialData::RegisterCategory("Checkpoint", ezSpatialData::Flags::None);
}

ezAiPerceptionGenCheckpoint::~ezAiPerceptionGenCheckpoint() = default;

void ezAiPerceptionGenCheckpoint::UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& perceptions = static_cast<InstanceData*>(pInstanceData)->m_Perceptions;
  perceptions.Clear();

  ezWorld* pWorld = ctx.m_Owner.GetWorld();
  const ezVec3 c = ctx.m_Owner.GetGlobalPosition();

  ezHybridArray<ezGameObject*, 32> checkpoints;

  ezSpatialSystem::QueryParams param;
  param.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  pWorld->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(c, m_fRadius), param, checkpoints);

  for (ezUInt32 i = 0; i < checkpoints.GetCount(); ++i)
  {
    perceptions.ExpandAndGetRef().m_vGlobalPosition = checkpoints[i]->GetGlobalPosition();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenTarget::ezAiPerceptionGenTarget() = default;
ezAiPerceptionGenTarget::~ezAiPerceptionGenTarget() = default;

void ezAiPerceptionGenTarget::UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& targets = static_cast<InstanceData*>(pInstanceData)->m_Perceptions;

  ezWorld* pWorld = ctx.m_Owner.GetWorld();
  const ezTime tNow = ctx.m_Now;
  const ezVec3 vOwnerPos = ctx.m_Owner.GetGlobalPosition();

  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    const void* pSensorData = nullptr;
    const ezAiSensorSpatial* pSensor = static_cast<const ezAiSensorSpatial*>(ctx.m_SensorManager.GetSensor(uiSensorIndex, pSensorData));
    pSensor->RetrieveSensations(ctx, pSensorData, detectedObjects);
  }

  for (auto& target : targets)
  {
    target.m_bVisible = false;
  }

  for (ezGameObjectHandle hObj : detectedObjects)
  {
    ezGameObject* pObj = nullptr;
    if (!pWorld->TryGetObject(hObj, pObj))
      continue;

    // the same object may be detected by several sensors
    ezAiPerceptionTarget* pTarget = nullptr;
    for (auto& target : targets)
    {
      if (target.m_hTarget == hObj)
      {
        pTarget = &target;
        break;
      }
    }

    if (pTarget == nullptr)
    {
      pTarget = &targets.ExpandAndGetRef();
      pTarget->m_hTarget = hObj;
    }

    pTarget->m_vGlobalPosition = pObj->GetGlobalPosition();
    pTarget->m_bVisible = true;
    pTarget->m_LastSeen = tNow;
  }

  for (ezUInt32 i = targets.GetCount(); i > 0; --i)
  {
    ezAiPerceptionTarget& target = targets[i - 1];

    ezGameObject* pObj = nullptr;
    // an inactive object is e.g. the marker of a dead character
    if (!pWorld->TryGetObject(target.m_hTarget, pObj) || !pObj->IsActive() || tNow - target.m_LastSeen > m_MemoryDuration)
    {
      targets.RemoveAtAndCopy(i - 1);
      continue;
    }

    target.m_fDistance = (target.m_vGlobalPosition - vOwnerPos).GetLength();
  }
}

void ezAiPerceptionGenTarget::FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const
{
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    ref_SensorManager.FlagAsNeeded(uiSensorIndex);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenHint::ezAiPerceptionGenHint() = default;
ezAiPerceptionGenHint::~ezAiPerceptionGenHint() = default;

void ezAiPerceptionGenHint::UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& hints = static_cast<InstanceData*>(pInstanceData)->m_Perceptions;
  hints.Clear();

  ezWorld* pWorld = ctx.m_Owner.GetWorld();
  const ezVec3 vOwnerPos = ctx.m_Owner.GetGlobalPosition();

  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    const void* pSensorData = nullptr;
    const ezAiSensorSpatial* pSensor = static_cast<const ezAiSensorSpatial*>(ctx.m_SensorManager.GetSensor(uiSensorIndex, pSensorData));
    pSensor->RetrieveSensations(ctx, pSensorData, detectedObjects);
  }

  if (detectedObjects.IsEmpty())
    return;

  ezHybridArray<ezGameObjectHandle, 32> directObjects;
  for (ezUInt32 uiSensorIndex : m_DirectSensors.GetConnections())
  {
    const void* pSensorData = nullptr;
    const ezAiSensorSpatial* pSensor = static_cast<const ezAiSensorSpatial*>(ctx.m_SensorManager.GetSensor(uiSensorIndex, pSensorData));
    pSensor->RetrieveSensations(ctx, pSensorData, directObjects);
  }

  // whether the source or any object below it is detected directly
  auto IsSourceDetected = [&](ezGameObjectHandle hSource)
  {
    for (ezGameObjectHandle hDirect : directObjects)
    {
      const ezGameObject* pObj = nullptr;
      if (!pWorld->TryGetObject(hDirect, pObj))
        continue;

      for (; pObj != nullptr; pObj = pObj->GetParent())
      {
        if (pObj->GetHandle() == hSource)
          return true;
      }
    }

    return false;
  };

  float fMaxPriority = -ezMath::MaxValue<float>();

  for (ezGameObjectHandle hObj : detectedObjects)
  {
    ezGameObject* pObj = nullptr;
    if (!pWorld->TryGetObject(hObj, pObj))
      continue;

    const ezAiMarkerComponent* pMarker = nullptr;
    if (!pObj->TryGetComponentOfBaseType(pMarker) || !pMarker->IsActive())
      continue;

    // the same object may be detected by several sensors
    bool bKnown = false;
    for (const auto& hint : hints)
    {
      if (hint.m_hMarker == hObj)
      {
        bKnown = true;
        break;
      }
    }

    if (bKnown)
      continue;

    const ezGameObjectHandle hSource = pMarker->GetSourceObject();
    if (IsSourceDetected(hSource))
      continue;

    auto& hint = hints.ExpandAndGetRef();
    hint.m_hMarker = hObj;
    hint.m_hSource = hSource;
    hint.m_vGlobalPosition = pObj->GetGlobalPosition();
    hint.m_vSourcePosition = hint.m_vGlobalPosition;
    hint.m_fDistance = (hint.m_vGlobalPosition - vOwnerPos).GetLength();
    hint.m_fPriority = pMarker->m_fPriority;

    const ezGameObject* pSource = nullptr;
    if (pWorld->TryGetObject(hSource, pSource))
    {
      hint.m_vSourcePosition = pSource->GetGlobalPosition();
    }

    fMaxPriority = ezMath::Max(fMaxPriority, hint.m_fPriority);
  }

  for (ezUInt32 i = hints.GetCount(); i > 0; --i)
  {
    if (hints[i - 1].m_fPriority < fMaxPriority)
    {
      hints.RemoveAtAndCopy(i - 1);
    }
  }
}

void ezAiPerceptionGenHint::FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const
{
  for (ezUInt32 uiSensorIndex : m_Sensors.GetConnections())
  {
    ref_SensorManager.FlagAsNeeded(uiSensorIndex);
  }

  for (ezUInt32 uiSensorIndex : m_DirectSensors.GetConnections())
  {
    ref_SensorManager.FlagAsNeeded(uiSensorIndex);
  }
}
