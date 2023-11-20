#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/HybridArray.h>

class ezAiControllerDesc;
class ezAiSensorManager;

/// Updates only those perception generators of one agent that are needed, and gives access to their perceptions.
///
/// The generators themselves are owned by the ezAiControllerDesc, this only tracks the per-agent state.
///
/// Per decision update: behaviors call FlagAsNeeded() with their input pins, then FlagNeededSensors() and
/// UpdateNeededPerceptions() are called, then behaviors read the results with GetPerceptions().
class EZ_AIPLUGIN_DLL ezAiPerceptionManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiPerceptionManager);

public:
  ezAiPerceptionManager();
  ~ezAiPerceptionManager();

  /// instanceData must stay valid until Deinitialize() is called.
  void Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData);
  void Deinitialize();

  /// Marks the generators connected to the pin for update in the current decision update.
  void FlagAsNeeded(const ezAiInputPin& pin);

  /// Lets the flagged generators flag the sensors they need.
  void FlagNeededSensors(ezAiSensorManager& ref_SensorManager);

  /// Updates the flagged generators and resets all flags for the next decision update.
  void UpdateNeededPerceptions(const ezAiContext& ctx);

  /// Whether any generator connected to the pin has perceptions, as of its last update.
  bool HasPerceptions(const ezAiInputPin& pin) const;

  /// Forwards the message to all generators (see ezAiPerceptionGenerator::HandleMessage()), whether they are needed or not.
  void HandleMessage(const ezAiContext& ctx, const ezMessage& msg);

  /// Appends the perceptions of all generators connected to the pin, as of their last update.
  void GetPerceptions(const ezAiInputPin& pin, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const;

  /// Appends the perceptions of all generators, as of their last update. For debug output.
  void GetAllPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const;

private:
  void* GetInstanceData(ezUInt32 uiGeneratorIdx) const;

  const ezAiControllerDesc* m_pDesc = nullptr;
  ezByteBlobPtr m_InstanceData;
  ezUInt32 m_uiUpdateCount = 1;
  ezHybridArray<ezUInt32, 8> m_NeededInUpdate; ///< Per generator. Equals m_uiUpdateCount if the generator is needed in the current decision update.
};
