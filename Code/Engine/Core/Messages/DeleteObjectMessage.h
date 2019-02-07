#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezMsgDeleteGameObject : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDeleteGameObject, ezMessage);

  bool m_bCancel = false;
};

