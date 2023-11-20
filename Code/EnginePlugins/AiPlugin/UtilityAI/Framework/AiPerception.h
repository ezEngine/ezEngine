#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>

/// Base class for a piece of information that an NPC knows about its surroundings, e.g. 'there is an enemy at X'.
///
/// Perceptions are produced by an ezAiPerceptionGenerator and read by behaviors to compute their scores.
/// Behaviors get them through an ezAiInputPin that accepts one perception type, and cast them to that type.
class EZ_AIPLUGIN_DLL ezAiPerception : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerception, ezReflectedClass);

public:
  ezAiPerception() = default;
  ~ezAiPerception() = default;

  /// Returns a copy, for code that needs the perception after the next perception update (which invalidates all perceptions).
  ///
  /// Every concrete perception type implements this, usually with EZ_AI_PERCEPTION_CLONE.
  virtual ezUniquePtr<ezAiPerception> Clone() const = 0;

  /// For debug output: the position the perception is about (e.g. the target), if it has one.
  virtual bool GetDebugPosition(ezVec3& out_vPosition) const { return false; }

  /// For debug output and recordings (ezAiDecisionRecorder): a short description of the state, e.g. whether a target is visible.
  virtual void GetDebugDesc(ezStringBuilder& out_sText) const {}
};

/// Implements ezAiPerception::Clone() with the copy constructor.
#define EZ_AI_PERCEPTION_CLONE(OwnType)                      \
  virtual ezUniquePtr<ezAiPerception> Clone() const override \
  {                                                          \
    return EZ_DEFAULT_NEW(OwnType, *this);                   \
  }
