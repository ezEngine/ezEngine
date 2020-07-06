#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>
#include <Foundation/Communication/Message.h>

struct EZ_RMLUIPLUGIN_DLL ezMsgRmlUiReload : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRmlUiReload, ezMessage);
};
