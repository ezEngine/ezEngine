#pragma once

#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>
#include <Core/World/Declarations.h>

struct EZ_CORE_DLL ezTriggerState
{
  enum Enum
  {
    Activated,      ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,     ///< The trigger is active for more than one frame now
    Deactivated,    ///< The trigger was just deactivated (left area, key released, etc.)
  };
};

struct EZ_CORE_DLL ezTriggerMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezTriggerMessage, ezMessage);

  /// Identifies what the message should trigger. Only stores the hashed string, because one should only check for equality with some expected string. Use ezTempHashedString::GetHash() to assign and compare the value.
  ezUInt32 m_UsageStringHash;

  ezTriggerState::Enum m_TriggerState;

  /// For things that may have an analog trigger 'strength', e.g. for input messages
  ezVariant m_TriggerValue;
};
