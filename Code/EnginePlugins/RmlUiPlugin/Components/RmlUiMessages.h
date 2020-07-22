#pragma once

#include <Foundation/Communication/Message.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiReload : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiReload, ezMessage);
};
