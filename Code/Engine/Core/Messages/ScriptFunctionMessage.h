#pragma once

#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all messages that scripts are allowed to send.
///
/// This common base class is used to filter out which messages to expose to scripts.
struct EZ_CORE_DLL ezScriptFunctionMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezScriptFunctionMessage, ezMessage);


};

