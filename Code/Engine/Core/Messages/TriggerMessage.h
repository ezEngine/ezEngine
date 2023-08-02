#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezTriggerState
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Activated,   ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,  ///< The trigger is active for more than one frame now.
    Deactivated, ///< The trigger was just deactivated (left area, key released, etc.)

    Default = Activated
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezTriggerState);

/// \brief For internal use by components to trigger some known behavior. Usually components will post this message to themselves with a
/// delay, e.g. to trigger self destruction.
struct EZ_CORE_DLL ezMsgComponentInternalTrigger : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgComponentInternalTrigger, ezMessage);

  /// Identifies what the message should trigger.
  ezHashedString m_sMessage;

  ezInt32 m_iPayload = 0;
};

/// \brief Sent when something enters or leaves a trigger
struct EZ_CORE_DLL ezMsgTriggerTriggered : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTriggerTriggered, ezEventMessage);

  /// Identifies what the message should trigger.
  ezHashedString m_sMessage;

  /// Messages are only sent for 'entered' ('Activated') and 'left' ('Deactivated')
  ezEnum<ezTriggerState> m_TriggerState;

  /// The object that entered the trigger volume.
  ezGameObjectHandle m_hTriggeringObject;
};
