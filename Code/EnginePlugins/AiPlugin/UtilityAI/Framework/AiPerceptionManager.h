#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>

class ezGameObject;
class ezAiSensorManager;

class EZ_AIPLUGIN_DLL ezAiPerceptionManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiPerceptionManager);

public:
  ezAiPerceptionManager();
  ~ezAiPerceptionManager();

  void FlagPerceptionTypeAsNeeded(ezStringView sPerceptionType);

  void AddGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator);

  void FlagNeededSensors(ezAiSensorManager& ref_SensorManager);

  void UpdateNeededPerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager);

  bool HasPerceptionsOfType(ezStringView sPerceptionType) const;
  void GetPerceptionsOfType(ezStringView sPerceptionType, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const;

private:
  struct PerceptionInfo
  {
    ezUniquePtr<ezAiPerceptionGenerator> m_pPerceptionGenerator;
    ezUInt32 m_uiNeededInUpdate = 0;
  };

  ezUInt32 m_uiUpdateCount = 1;
  ezHybridArray<PerceptionInfo, 12> m_PerceptionGenerators;
};
