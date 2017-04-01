#pragma once

#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>
#include <Core/World/Declarations.h>

struct EZ_CORE_DLL ezTriggerState
{
  enum Enum
  {
    Activated,      ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,     ///< The trigger is active for more than one frame now.
    Deactivated,    ///< The trigger was just deactivated (left area, key released, etc.)
  };
};

/// \brief Used by components to signal that they have been triggered.
struct EZ_CORE_DLL ezTriggerMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezTriggerMessage, ezMessage);

  /// Identifies what the message should trigger. Only stores the hashed string, because one should only check for equality with some expected string. Use ezTempHashedString::GetHash() to assign and compare the value.
  ezUInt32 m_UsageStringHash;

  ezTriggerState::Enum m_TriggerState;

  /// For things that may have an analog trigger 'strength', e.g. for input messages
  ezVariant m_TriggerValue;

  /// If available, the object that's responsible for the event. E.g. for a physical trigger, the object that entered the trigger volume.
  ezGameObjectHandle m_hTriggeringObject;
};

/// \brief For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom message for more elaborate cases where a string is not sufficient information.
/// Also be aware that passing this message is not the most efficient due to the string copy overhead.
struct EZ_CORE_DLL ezUserTriggerMessage : public ezScriptMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezUserTriggerMessage, ezScriptMessage);

  /// A custom string to identify the intent.
  ezString m_sMessage;
};
