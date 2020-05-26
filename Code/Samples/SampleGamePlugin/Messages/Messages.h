#pragma once

#include <Foundation/Communication/Message.h>

// BEGIN-DOCS-CODE-SNIPPET: message-decl
struct ezMsgSetText : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetText, ezMessage);

  ezString m_sText;
};
// END-DOCS-CODE-SNIPPET
