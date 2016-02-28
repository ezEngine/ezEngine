#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

typedef ezComponentManager<class ezCameraComponent> ezCameraComponentManager;

class EZ_GAMEUTILS_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

public:
  ezCameraComponent();
  ~ezCameraComponent();

  static ezCameraComponent* s_pCurrent;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual Initialization Initialize() override;


  // ************************************* PROPERTIES ***********************************

private:


};
