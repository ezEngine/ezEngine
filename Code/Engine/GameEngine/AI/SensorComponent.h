#pragma once

#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezSensorComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezSensorComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

public:
  ezSensorComponent();
  ~ezSensorComponent();
};
