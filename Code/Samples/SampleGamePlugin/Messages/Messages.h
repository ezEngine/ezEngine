#pragma once

#include <Foundation/Communication/Message.h>

struct ezMsgSetText : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetText, ezMessage);

  ezString m_sText;
};
