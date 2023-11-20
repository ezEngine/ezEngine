#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>

class ezGameObject;
class ezAiSensorManager;

class EZ_AIPLUGIN_DLL ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenerator() = default;
  virtual ~ezAiPerceptionGenerator() = default;

  virtual ezStringView GetPerceptionType() = 0;
  virtual void UpdatePerceptions(ezGameObject& owner, const ezAiSensorManager& ref_SensorManager) = 0;
  virtual bool HasPerceptions() const = 0;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const = 0;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) = 0;
};
