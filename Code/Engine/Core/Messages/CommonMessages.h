#pragma once

#include <Foundation/Communication/Message.h>
#include <Core/World/Declarations.h>

/// \brief Common message for components that can be toggled between playing and paused states
struct EZ_CORE_DLL ezMsgSetPlaying : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetPlaying, ezMessage);

  bool m_bPlay = true;
};


