#pragma once

#include <Foundation/Basics.h>

typedef ezUInt16 ezMessageId;

class EZ_FOUNDATION_DLL ezMessage
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezMessageId GetId()
  {
    return m_Id;
  }

  EZ_FORCE_INLINE static ezMessageId GetNextMsgId()
  {
    return s_uiNextMsgId++;
  }

protected:
  ezMessageId m_Id;
  static ezMessageId s_uiNextMsgId;
};

#define EZ_DECLARE_MESSAGE_TYPE(messageType) \
  public: \
    static ezMessageId MSG_ID; \
    EZ_FORCE_INLINE messageType() { m_Id = messageType::MSG_ID; }

#define EZ_IMPLEMENT_MESSAGE_TYPE(messageType) \
  ezMessageId messageType::MSG_ID = ezMessage::GetNextMsgId();
