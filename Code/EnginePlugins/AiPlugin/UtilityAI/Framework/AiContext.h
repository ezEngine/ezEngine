#pragma once

#include <Foundation/Time/Time.h>

#include <AiPlugin/AiPluginDLL.h>

class ezGameObject;
class ezLogInterface;
class ezAiSensorManager;
class ezAiPerceptionManager;
class ezAiConsiderationManager;
class ezAiActionQueue;

/// Everything that sensors, perception generators and behaviors may access while they are executed for one agent.
///
/// Created by ezAiComponent on the stack for every update. Sensors, generators and behaviors are shared between agents,
/// so they must not store any of this, per-agent state belongs into their instance data instead.
struct ezAiContext
{
  ezGameObject& m_Owner;
  ezTime m_Now; ///< Accumulated world time.
  ezLogInterface* m_pLog = nullptr;

  ezAiSensorManager& m_SensorManager;
  ezAiPerceptionManager& m_PerceptionManager;
  ezAiConsiderationManager& m_ConsiderationManager;
  ezAiActionQueue& m_ActionQueue;
};
