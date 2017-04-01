#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>

typedef ezUInt16 ezMessageId;

/// \brief Base class for all message types. Each message type has it's own id which is used to dispatch messages efficiently.
///
/// To implement a custom message type derive from ezMessage and add EZ_DECLARE_MESSAGE_TYPE to the type declaration.
/// EZ_IMPLEMENT_MESSAGE_TYPE needs to be added to a cpp.
/// \see ezRTTI
/// \todo implement serialization
class EZ_FOUNDATION_DLL ezMessage : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMessage, ezReflectedClass);

public:
  ezMessage()
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive = false;
#endif
  }

  virtual ~ezMessage() {}

  /// \brief Returns a copy of this allocated with the given allocator. This method is automatically implemented by adding EZ_DECLARE_MESSAGE_TYPE.
  virtual ezMessage* Clone(ezAllocatorBase* pAllocator) const = 0;

  /// \brief Derived message types can override this method to influence sorting order. Smaller keys are processed first.
  virtual ezInt32 GetSortingKey() const { return 0; }

  //virtual void Serialize(ezStreamWriter& stream) const;
  //virtual ezResult Deserialize(ezStreamReader& stream);

  /// \brief Returns the id for this message type.
  EZ_ALWAYS_INLINE ezMessageId GetId() const
  {
    return m_Id;
  }

  /// \brief Returns the size in byte of this message.
  EZ_ALWAYS_INLINE ezUInt16 GetSize() const
  {
    return m_uiSize;
  }

  /// \brief Calculates a murmur hash of the message.
  EZ_ALWAYS_INLINE ezUInt32 GetHash() const
  {
    return ezHashing::MurmurHash(this, m_uiSize);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  /// set to true while debugging a message routing problem
  /// if the message is not delivered to any recipient at all, information about why that is will be written to ezLog
  bool m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive;
#endif

protected:
  EZ_ALWAYS_INLINE static ezMessageId GetNextMsgId()
  {
    return s_uiNextMsgId++;
  }

  ezMessageId m_Id;
  ezUInt16 m_uiSize;

  static ezMessageId s_uiNextMsgId;
};

/// \brief Add this macro to the declaration of your custom message type.
#define EZ_DECLARE_MESSAGE_TYPE(messageType, baseType) \
  private: \
    EZ_ADD_DYNAMIC_REFLECTION(messageType, baseType); \
    static ezMessageId MSG_ID; \
  public: \
    static ezMessageId GetMsgId() \
    { \
      static ezMessageId id = ezMessage::GetNextMsgId(); \
      return id; \
    } \
    EZ_ALWAYS_INLINE messageType() \
    { \
      m_Id = messageType::MSG_ID; \
      m_uiSize = sizeof(messageType); \
    } \
    \
    virtual ezMessage* Clone(ezAllocatorBase* pAllocator) const override \
    { \
      return EZ_NEW(pAllocator, messageType, *static_cast<const messageType*>(this)); \
    }

/// \brief Implements the given message type. Add this macro to a cpp outside of the type declaration.
#define EZ_IMPLEMENT_MESSAGE_TYPE(messageType) \
  ezMessageId messageType::MSG_ID = messageType::GetMsgId();

