#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Input/Declarations.h>

typedef ezComponentManagerSimple<class ezInputComponent, true> ezInputComponentManager;

struct EZ_GAMEUTILS_DLL ezInputComponentMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezInputComponentMessage);

  const char* m_szAction;
  ezKeyState::Enum m_State;
  float m_fValue;
};

class EZ_GAMEUTILS_DLL ezInputComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezInputComponent, ezComponent, ezInputComponentManager);

public:
  ezInputComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  ezString m_sInputSet;

};
