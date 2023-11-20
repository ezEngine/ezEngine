#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/HashedString.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>

/// Wraps the ezSensorComponent(s) on a child object of the NPC (e.g. a vision cone).
///
/// UpdateSensor() runs the sensor checks manually, so the AI decides when (and whether) the potentially expensive check happens.
/// The sensor components should therefore use the update rate 'Never'. If they don't, a warning is logged and the update rate is changed.
class EZ_AIPLUGIN_DLL ezAiSensorSpatial : public ezAiSensor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiSensorSpatial, ezAiSensor);

public:
  ezAiSensorSpatial();
  ezAiSensorSpatial(const ezHashedString& sObjectName);
  ~ezAiSensorSpatial();

  virtual void UpdateSensor(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  /// Appends the objects that all active sensor components detected during their last check.
  ///
  /// pInstanceData is the one returned by ezAiSensorManager::GetSensor().
  void RetrieveSensations(const ezAiContext& ctx, const void* pInstanceData, ezDynamicArray<ezGameObjectHandle>& out_Sensations) const;

  /// Name of the child object (searched recursively) that holds the sensor components.
  ezHashedString m_sObjectName; // [ property ]

private:
  struct InstanceData
  {
    ezGameObjectHandle m_hSensorObject;
  };
};
