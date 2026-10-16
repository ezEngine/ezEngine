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

  // the sensor components update themselves (see their update rate), RetrieveSensations() reads their last results
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
