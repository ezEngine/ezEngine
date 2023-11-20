#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

struct ezInstanceDataDesc;

/// Gathers raw information about the world (e.g. which objects are visible), which perception generators then interpret.
///
/// Perception generators reference sensors through an ezAiInputPin and cast them to the concrete type to read the results,
/// so there is no common interface for that.
///
/// A sensor object is shared by all agents of the same type and must not be modified after creation.
/// Per-agent state belongs into the instance data, see GetInstanceDataDesc().
class EZ_AIPLUGIN_DLL ezAiSensor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiSensor, ezReflectedClass);

public:
  ezAiSensor() = default;
  ~ezAiSensor() = default;

  /// Only for display (node subtitle in the editor), doesn't need to be unique.
  ezHashedString m_sName; // [ property ]

  /// Called during a decision update, only if a perception generator flagged this sensor as needed.
  virtual void UpdateSensor(const ezAiContext& ctx, void* pInstanceData) const = 0;

  /// Returns false, if the sensor doesn't need instance data. Then pInstanceData is null in all calls.
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }
};
