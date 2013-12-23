#pragma once

#include <Foundation/Algorithm/Hashing.h>

typedef ezUInt16 ezMessageId;

/// \todo document and test
class EZ_FOUNDATION_DLL ezMessage
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezMessageId GetId() const
  {
    return m_Id;
  }

  EZ_FORCE_INLINE ezUInt16 GetSize() const
  {
    return m_uiSize;
  }

  EZ_FORCE_INLINE ezUInt32 GetHash() const 
  { 
    return ezHashing::MurmurHash(this, m_uiSize);
  }

  EZ_FORCE_INLINE static ezMessageId GetNextMsgId()
  {
    return s_uiNextMsgId++;
  }

protected:
  ezMessageId m_Id;
  ezUInt16 m_uiSize;
  static ezMessageId s_uiNextMsgId;
};

#define EZ_DECLARE_MESSAGE_TYPE(messageType) \
  public: \
    static ezMessageId MSG_ID; \
    EZ_DECLARE_POD_TYPE(); \
    EZ_FORCE_INLINE messageType() { \
      ezMemoryUtils::ZeroFill(this); \
      m_Id = messageType::MSG_ID; \
      m_uiSize = sizeof(messageType); \
    }

#define EZ_IMPLEMENT_MESSAGE_TYPE(messageType) \
  ezMessageId messageType::MSG_ID = ezMessage::GetNextMsgId();

