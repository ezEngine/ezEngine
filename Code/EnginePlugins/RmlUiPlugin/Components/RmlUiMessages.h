#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Communication/Message.h>

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiReload : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiReload, ezMessage);
};

//////////////////////////////////////////////////////////////////////////

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiEvent : ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiEvent, ezMessage);

  ezHashedString m_sIdentifier;
  ezHashedString m_sType;
};
