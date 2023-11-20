#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/HashedString.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>

class EZ_AIPLUGIN_DLL ezAiSensorSpatial : public ezAiSensor
{
public:
  ezAiSensorSpatial(ezTempHashedString sObjectName);
  ~ezAiSensorSpatial();

  virtual void UpdateSensor(ezGameObject& owner) override;
  void RetrieveSensations(ezGameObject& owner, ezDynamicArray<ezGameObjectHandle>& out_Sensations) const;

  ezTempHashedString m_sObjectName;

private:
  ezGameObjectHandle m_hSensorObject;
};
