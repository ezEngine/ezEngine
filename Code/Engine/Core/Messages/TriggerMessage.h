#pragma once

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

  /// Identifies what the message should trigger. Only stores the hashed string, because one should only check for equality with some
  /// expected string. Use ezTempHashedString::ComputeHash() to assign and compare the value.
  ezUInt32 m_uiUsageStringHash = 0;
};
