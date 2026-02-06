#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/Messages/EventMessage.h>

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiReload : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiReload, ezMessage);
};

//////////////////////////////////////////////////////////////////////////

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiEvent : ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiEvent, ezEventMessage);

  ezHashedString m_sIdentifier;
  ezHashedString m_sType;
};
