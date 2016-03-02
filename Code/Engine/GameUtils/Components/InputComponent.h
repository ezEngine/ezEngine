#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Input/Declarations.h>

typedef ezComponentManagerSimple<class ezInputComponent, true> ezInputComponentManager;

/// \brief The message type that ezInputComponent broadcasts to all other components on the same node for every input event.
struct EZ_GAMEUTILS_DLL ezInputComponentMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezInputComponentMessage);

  const char* m_szAction;
  ezKeyState::Enum m_State;
  float m_fValue; ///< How much an (analog) button is pressed
};

/// \brief Which types of input events are broadcast
struct EZ_GAMEUTILS_DLL ezInputMessageGranularity
{
  typedef ezInt8 StorageType;

  /// \brief Which types of input events are broadcast
  enum Enum
  {
    PressOnly, ///< Key pressed events are sent, but nothing else
    PressAndRelease, ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well
    PressReleaseDownAndUp, ///< All input events are sent, including 'key up' events. Attention: This means every frame for every input event one message is sent!

    Default = PressReleaseAndDown
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEUTILS_DLL, ezInputMessageGranularity);

/// \brief This component polls all input events from the given input set every frame and broadcasts the information to components on the same game object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type ezInputComponentMessage.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released, down or up) messages are sent.
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
  ezEnum<ezInputMessageGranularity> m_Granularity;
};
