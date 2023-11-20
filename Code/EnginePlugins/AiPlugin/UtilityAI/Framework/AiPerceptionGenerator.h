#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiContext.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

class ezAiControllerDesc;
class ezAiSensorManager;
struct ezMessage;
struct ezInstanceDataDesc;

/// Declares the perception type that an ezAiPerceptionGenerator type produces. Required on every non-abstract generator type.
///
/// Behaviors connect to generators through an ezAiInputPin that accepts a perception type, see ezAiInputPinAttribute.
class EZ_AIPLUGIN_DLL ezAiPerceptionTypeAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionTypeAttribute, ezPropertyAttribute);

public:
  ezAiPerceptionTypeAttribute() = default;
  ezAiPerceptionTypeAttribute(const char* szPerceptionType)
    : m_sPerceptionType(szPerceptionType)
  {
  }

  ezStringView GetPerceptionTypeName() const { return m_sPerceptionType; }

private:
  ezUntrackedString m_sPerceptionType;
};

/// Produces perceptions of one type, typically by evaluating sensor data.
///
/// The produced type is declared with an ezAiPerceptionTypeAttribute on the generator type.
///
/// Generators only update when a behavior that is connected to them flagged them as needed in the current decision update.
/// Otherwise the perceptions from the last update are kept, so they can be outdated.
///
/// A generator object is shared by all agents of the same type and must not be modified after creation.
/// The produced perceptions and all other per-agent state belong into the instance data, see GetInstanceDataDesc().
class EZ_AIPLUGIN_DLL ezAiPerceptionGenerator : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionGenerator, ezReflectedClass);

public:
  ezAiPerceptionGenerator() = default;
  ~ezAiPerceptionGenerator() = default;

  /// Called by ezAiControllerDesc::Finalize(), after the input pins were validated. The last chance to modify the generator.
  virtual void OnFinalize(const ezAiControllerDesc& desc) {}

  /// The perception type declared with ezAiPerceptionTypeAttribute on the given generator type, or nullptr if there is none.
  static const ezRTTI* GetPerceptionType(const ezRTTI* pGeneratorType);

  /// Recomputes the perceptions. The sensors flagged in FlagNeededSensors() have been updated right before.
  virtual void UpdatePerceptions(const ezAiContext& ctx, void* pInstanceData) const = 0;

  virtual bool HasPerceptions(const void* pInstanceData) const = 0;

  /// Appends pointers to the stored perceptions. They stay valid until the next UpdatePerceptions().
  virtual void GetPerceptions(const void* pInstanceData, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const = 0;

  /// Calls ezAiSensorManager::FlagAsNeeded() for every sensor that UpdatePerceptions() reads.
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) const = 0;

  /// Called for messages that the AI component receives (e.g. ezMsgDamage), to push events into the perception.
  ///
  /// Since perceptions are only updated when needed, generators should store the information and evaluate it in UpdatePerceptions().
  virtual void HandleMessage(const ezAiContext& ctx, void* pInstanceData, const ezMessage& msg) const {}

  /// Returns false, if the generator doesn't need instance data. Then pInstanceData is null in all calls.
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }
};
