#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <GameEngine/AI/SensorComponent.h>

ezAiSensorSpatial::ezAiSensorSpatial(ezTempHashedString sObjectName)
{
  m_sObjectName = sObjectName;
}

ezAiSensorSpatial::~ezAiSensorSpatial() = default;

void ezAiSensorSpatial::UpdateSensor(ezGameObject& owner)
{
  if (m_hSensorObject.IsInvalidated())
  {
    ezGameObject* pSensors = owner.FindChildByName(m_sObjectName);

    if (pSensors == nullptr)
      return;

    m_hSensorObject = pSensors->GetHandle();
  }

  ezWorld* pWorld = owner.GetWorld();
  ezGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(m_hSensorObject, pSensors))
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
    pSensor->RunSensorCheck(pPhysicsWorldModule, objectsInSensorVolume, detectedObjects, false);
  }
}

void ezAiSensorSpatial::RetrieveSensations(ezGameObject& owner, ezDynamicArray<ezGameObjectHandle>& out_Sensations) const
{
  ezWorld* pWorld = owner.GetWorld();
  ezGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(m_hSensorObject, pSensors))
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
    ezArrayPtr<const ezGameObjectHandle> detections = pSensor->GetLastDetectedObjects();
    out_Sensations.PushBackRange(detections);
  }

  if (out_Sensations.GetCount() > 16)
  {
    ezLog::Warning("Much sensor input: {} items", out_Sensations.GetCount());
  }
}
