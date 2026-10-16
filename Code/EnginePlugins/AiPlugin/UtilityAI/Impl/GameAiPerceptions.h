#pragma once

#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>

class EZ_AIPLUGIN_DLL ezAiPerceptionPOI : public ezAiPerception
{
public:
  ezAiPerceptionPOI() = default;
  ~ezAiPerceptionPOI() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};


class EZ_AIPLUGIN_DLL ezAiPerceptionWander : public ezAiPerception
{
public:
  ezAiPerceptionWander() = default;
  ~ezAiPerceptionWander() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};

class EZ_AIPLUGIN_DLL ezAiPerceptionCheckpoint : public ezAiPerception
{
public:
  ezAiPerceptionCheckpoint() = default;
  ~ezAiPerceptionCheckpoint() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};

/// A (potentially hostile) object that was seen or is remembered.
class EZ_AIPLUGIN_DLL ezAiPerceptionTarget : public ezAiPerception
{
public:
  ezAiPerceptionTarget() = default;
  ~ezAiPerceptionTarget() = default;

  ezGameObjectHandle m_hTarget;
  ezVec3 m_vGlobalPosition = ezVec3::MakeZero(); ///< Current position if visible, otherwise the last known one.
  float m_fDistance = 0.0f;                      ///< Distance from the owner to m_vGlobalPosition at the time of the perception update.
  bool m_bVisible = false;                       ///< Whether a sensor currently detects the target.
  bool m_bAlerted = false;                       ///< The information comes from an event (e.g. damage) rather than from seeing the target.
  ezTime m_LastSeen;                             ///< Time of the last information update. Changes whenever new information arrives, can be used as an ID.
};
