#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/HybridArray.h>

class ezAiControllerDesc;

/// Updates only those sensors of one agent that are needed in the current decision update.
///
/// The sensors themselves are owned by the ezAiControllerDesc, this only tracks the per-agent state.
class EZ_AIPLUGIN_DLL ezAiSensorManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiSensorManager);

public:
  ezAiSensorManager();
  ~ezAiSensorManager();

  /// instanceData must stay valid until Deinitialize() is called.
  void Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData);
  void Deinitialize();

  /// Marks the sensor for update in the current decision update. uiSensorIndex is an index into ezAiControllerDesc::GetSensors(), typically from an ezAiInputPin. ezInvalidIndex is ignored.
  void FlagAsNeeded(ezUInt32 uiSensorIndex);

  /// Updates the flagged sensors and resets all flags for the next decision update.
  void UpdateNeededSensors(const ezAiContext& ctx);

  /// Returns nullptr for ezInvalidIndex. The result has to be cast to the concrete sensor type.
  ///
  /// out_pInstanceData is the agent's instance data for that sensor, which has to be passed to the sensor's functions.
  const ezAiSensor* GetSensor(ezUInt32 uiSensorIndex, const void*& out_pInstanceData) const;

private:
  void* GetInstanceData(ezUInt32 uiSensorIdx) const;

  const ezAiControllerDesc* m_pDesc = nullptr;
  ezByteBlobPtr m_InstanceData;
  ezUInt32 m_uiUpdateCount = 1;
  ezHybridArray<ezUInt32, 4> m_NeededInUpdate; ///< Per sensor. Equals m_uiUpdateCount if the sensor is needed in the current decision update.
};
