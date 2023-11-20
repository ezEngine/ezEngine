#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <AiPlugin/UtilityAI/Framework/AiConsideration.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RefCounted.h>

/// The complete setup of one type of agent: its sensors, perception generators, behaviors and considerations.
///
/// Shared by all agents of that type, each agent only allocates the instance data (see AllocateInstanceData()).
/// Fill it with the Add functions, then call Finalize(). Afterwards it must not be modified anymore.
class EZ_AIPLUGIN_DLL ezAiControllerDesc : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiControllerDesc);

public:
  ezAiControllerDesc();
  ~ezAiControllerDesc();

  void AddSensor(ezUniquePtr<ezAiSensor>&& pSensor);
  void AddPerceptionGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator);
  void AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior);
  void AddConsideration(ezUniquePtr<ezAiConsideration>&& pConsideration);

  /// Validates the input pins, calls ezAiPerceptionGenerator::OnFinalize() and computes the instance data layout.
  ///
  /// Pin connections that are out of range or reference a node of the wrong type are removed with a warning (see ezAiInputPin).
  void Finalize();

  /// Writes all sensors, perception generators, behaviors and considerations with their reflected properties.
  ///
  /// Only reflected properties are stored, members that are set up in code (e.g. in OnFinalize()) are not.
  ///
  /// The data is an ezAbstractObjectGraph with one node per object and a node named "root",
  /// which lists the object guids in the properties "Sensors", "PerceptionGenerators", "Behaviors" and "Considerations" (in order).
  /// The AI controller asset writes the same format.
  ezResult Serialize(ezStreamWriter& inout_stream) const;

  /// Reads the data written by Serialize(). Has to be called on an empty desc, before Finalize().
  ///
  /// Objects of unknown types or of the wrong base type are skipped with a warning.
  ezResult Deserialize(ezStreamReader& inout_stream);

  bool IsFinalized() const { return m_bFinalized; }

  /// Allocates and constructs the instance data of all sensors, generators, behaviors and considerations for one agent.
  ezBlob AllocateInstanceData() const;
  void DeallocateInstanceData(ezBlob& ref_blob) const;

  struct Sensor
  {
    ezUniquePtr<ezAiSensor> m_pSensor;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  struct PerceptionGenerator
  {
    ezUniquePtr<ezAiPerceptionGenerator> m_pGenerator;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  struct Behavior
  {
    ezUniquePtr<ezAiBehavior> m_pBehavior;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  struct Consideration
  {
    ezUniquePtr<ezAiConsideration> m_pConsideration;
    ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
  };

  ezArrayPtr<const Sensor> GetSensors() const { return m_Sensors; }
  ezArrayPtr<const PerceptionGenerator> GetPerceptionGenerators() const { return m_PerceptionGenerators; }
  ezArrayPtr<const Behavior> GetBehaviors() const { return m_Behaviors; }
  ezArrayPtr<const Consideration> GetConsiderations() const { return m_Considerations; }

private:
  void ValidateInputPins(ezReflectedClass* pNode) const;

  bool m_bFinalized = false;
  ezDynamicArray<Sensor> m_Sensors;
  ezDynamicArray<PerceptionGenerator> m_PerceptionGenerators;
  ezDynamicArray<Behavior> m_Behaviors;
  ezDynamicArray<Consideration> m_Considerations;
  ezInstanceDataAllocator m_InstanceDataAllocator;
};
