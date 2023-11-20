#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <GameEngine/AI/SensorComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiSensorSpatial, 1, ezRTTIDefaultAllocator<ezAiSensorSpatial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectName", m_sObjectName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiSensorSpatial::ezAiSensorSpatial() = default;

ezAiSensorSpatial::ezAiSensorSpatial(const ezHashedString& sObjectName)
{
  m_sObjectName = sObjectName;
}

ezAiSensorSpatial::~ezAiSensorSpatial() = default;

bool ezAiSensorSpatial::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezAiSensorSpatial::UpdateSensor(const ezAiContext& ctx, void* pInstanceData) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  if (pData->m_hSensorObject.IsInvalidated())
  {
    ezGameObject* pSensors = ctx.m_Owner.FindChildByName(m_sObjectName);

    if (pSensors == nullptr)
      return;

    pData->m_hSensorObject = pSensors->GetHandle();
  }

  ezWorld* pWorld = ctx.m_Owner.GetWorld();
  ezGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(pData->m_hSensorObject, pSensors))
    return;

  ezDynamicArray<ezSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;

  ezPhysicsWorldModuleInterface* pPhysicsWorldModule = pWorld->GetModule<ezPhysicsWorldModuleInterface>();

  ezHybridArray<ezGameObject*, 32> objectsInSensorVolume;
  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;

  for (auto pSensor : sensors)
  {
    if (!pSensor->IsActiveAndInitialized())
      continue;

    if (pSensor->GetUpdateRate() != ezUpdateRate::Never)
    {
      // the AI system decides when (and whether) a sensor check is done, an automatic update would only waste performance
      ezLog::Warning("Sensor component on '{}' has an update rate set, but is driven by the AI system. Update rate is set to 'Never'.", pSensors->GetName());
      pSensor->SetUpdateRate(ezUpdateRate::Never);
    }

    pSensor->RunSensorCheck(pPhysicsWorldModule, objectsInSensorVolume, detectedObjects, false);
  }
}

void ezAiSensorSpatial::RetrieveSensations(const ezAiContext& ctx, const void* pInstanceData, ezDynamicArray<ezGameObjectHandle>& out_Sensations) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  ezWorld* pWorld = ctx.m_Owner.GetWorld();
  ezGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(pData->m_hSensorObject, pSensors))
    return;

  ezDynamicArray<ezSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;

  for (auto pSensor : sensors)
  {
    if (!pSensor->IsActiveAndInitialized())
      continue;

    ezArrayPtr<const ezGameObjectHandle> detections = pSensor->GetLastDetectedObjects();
    out_Sensations.PushBackRange(detections);
  }

  if (out_Sensations.GetCount() > 16)
  {
    ezLog::Warning("Much sensor input: {} items", out_Sensations.GetCount());
  }
}
