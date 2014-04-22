#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/Stream.h>

typedef ezUInt16 ezMessageId;

/// \todo document and test
class EZ_FOUNDATION_DLL ezMessage
{
public:
  virtual ~ezMessage() {}

  virtual ezMessage* Clone(ezAllocatorBase* pAllocator) const = 0;

  /// \todo
  //virtual void Serialize(ezStreamWriterBase& stream) const;
  //virtual ezResult Deserialize(ezStreamReaderBase& stream);

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
  private: \
    static ezMessageId MSG_ID; \
  public: \
    static ezMessageId GetMsgId() \
    { \
      static ezMessageId id = ezMessage::GetNextMsgId(); \
      return id; \
    } \
    EZ_FORCE_INLINE messageType() \
    { \
      m_Id = messageType::MSG_ID; \
      m_uiSize = sizeof(messageType); \
    } \
    \
    virtual ezMessage* Clone(ezAllocatorBase* pAllocator) const EZ_OVERRIDE \
    { \
      return EZ_NEW(pAllocator, messageType)(*static_cast<const messageType*>(this)); \
    }

#define EZ_IMPLEMENT_MESSAGE_TYPE(messageType) \
  ezMessageId messageType::MSG_ID = messageType::GetMsgId();

