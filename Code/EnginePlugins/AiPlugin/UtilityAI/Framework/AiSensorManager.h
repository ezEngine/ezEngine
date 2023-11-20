#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>

class EZ_AIPLUGIN_DLL ezAiSensorManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiSensorManager);

public:
  ezAiSensorManager();
  ~ezAiSensorManager();

  void AddSensor(ezStringView sName, ezUniquePtr<ezAiSensor>&& pSensor);

  void FlagAsNeeded(ezStringView sName);

  void UpdateNeededSensors(ezGameObject& owner);

  const ezAiSensor* GetSensor(ezStringView sName) const;

private:
  struct SensorInfo
  {
    ezString m_sName;
    ezUniquePtr<ezAiSensor> m_pSensor;
    bool m_bActive = true;
    ezUInt32 m_uiNeededInUpdate = 0;
  };

  ezUInt32 m_uiUpdateCount = 1;
  ezHybridArray<SensorInfo, 2> m_Sensors;
};
