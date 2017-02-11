#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Input/Declarations.h>

typedef ezComponentManagerSimple<class ezInputComponent, ezComponentUpdateType::WhenSimulating> ezInputComponentManager;

/// \brief Which types of input events are broadcast
struct EZ_GAMEENGINE_DLL ezInputMessageGranularity
{
  typedef ezInt8 StorageType;

  /// \brief Which types of input events are broadcast
  enum Enum
  {
    PressOnly, ///< Key pressed events are sent, but nothing else
    PressAndRelease, ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well

    Default = PressReleaseAndDown
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezInputMessageGranularity);

/// \brief This component polls all input events from the given input set every frame and broadcasts the information to components on the same game object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type ezTriggerMessage.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released or down) messages are sent.
class EZ_GAMEENGINE_DLL ezInputComponent : public ezComponent
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
