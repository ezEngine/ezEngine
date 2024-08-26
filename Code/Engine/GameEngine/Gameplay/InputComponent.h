#pragma once

#include <Core/Input/Declarations.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using ezInputComponentManager = ezComponentManagerSimple<class ezInputComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Which types of input events are broadcast
struct EZ_GAMEENGINE_DLL ezInputMessageGranularity
{
  using StorageType = ezInt8;

  /// \brief Which types of input events are broadcast
  enum Enum
  {
    PressOnly,           ///< Key pressed events are sent, but nothing else
    PressAndRelease,     ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well

    Default = PressOnly
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezInputMessageGranularity);

/// \brief ezInputComponent raises this event when it detects input
struct EZ_GAMEENGINE_DLL ezMsgInputActionTriggered : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgInputActionTriggered, ezEventMessage);

  /// The input action string.
  ezHashedString m_sInputAction;

  /// The 'trigger state', depending on the key state and the configuration on the ezInputComponent
  ezEnum<ezTriggerState> m_TriggerState;

  /// For analog keys, how much they are pressed. Typically between 0 and 1.
  float m_fKeyPressValue;

private:
  const char* GetInputAction() const { return m_sInputAction; }
  void SetInputAction(const char* szInputAction) { m_sInputAction.Assign(szInputAction); }
};

/// \brief This component polls all input events from the given input set every frame and broadcasts the information to components on the same game
/// object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type ezMsgInputActionTriggered.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released or down) messages are sent.
class EZ_GAMEENGINE_DLL ezInputComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezInputComponent, ezComponent, ezInputComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezInputComponent

public:
  ezInputComponent();
  ~ezInputComponent();

  /// \brief Returns the amount to which szInputAction is active (0 to 1).
  ///
  /// If bOnlyKeyPressed is set to true, only key press events return a non-zero value,
  /// ie key down and key released events are ignored.
  float GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed = false) const; // [ scriptable ]

  ezString m_sInputSet;                                                                      // [ property ]
  ezEnum<ezInputMessageGranularity> m_Granularity;                                           // [ property ]
  bool m_bForwardToBlackboard = false;                                                       // [ property ]

protected:
  void Update();

  ezEventMessageSender<ezMsgInputActionTriggered> m_InputEventSender; // [ event ]
};
