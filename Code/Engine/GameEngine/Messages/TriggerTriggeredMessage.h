
#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>


/// \brief Sent when something enters or leaves the trigger
struct EZ_GAMEENGINE_DLL ezMsgTriggerTriggered : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTriggerTriggered, ezEventMessage);

  /// Identifies what the message should trigger. Only stores the hashed string, because one should only check for equality with some expected string. Use ezTempHashedString::GetHash() to assign and compare the value.
  ezUInt32 m_uiMessageStringHash;

  /// Messages are only sent for 'entered' ('Activated') and 'left' ('Deactivated')
  ezEnum<ezTriggerState> m_TriggerState;

  /// The object that entered the trigger volume.
  ezGameObjectHandle m_hTriggeringObject;
};
