#pragma once

#include <GameEngine/GameEngineDLL.h>

class ezGameObject;

class EZ_AIPLUGIN_DLL ezAiSensor
{
public:
  ezAiSensor() = default;
  virtual ~ezAiSensor() = default;

  virtual void UpdateSensor(ezGameObject& owner) = 0;
};
