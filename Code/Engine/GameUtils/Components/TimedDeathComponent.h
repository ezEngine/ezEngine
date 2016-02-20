#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Foundation/Time/Time.h>

typedef ezComponentManagerSimple<class ezTimedDeathComponent, true> ezTimedDeathComponentManager;

class EZ_GAMEUTILS_DLL ezTimedDeathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTimedDeathComponent, ezComponent, ezTimedDeathComponentManager);

public:
  ezTimedDeathComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************


};
