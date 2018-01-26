#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Reflection/Reflection.h>

typedef ezUInt16 ezMessageId;
class ezStreamWriter;
class ezStreamReader;

/// \brief Base class for all message types. Each message type has it's own id which is used to dispatch messages efficiently.
///
/// To implement a custom message type derive from ezMessage and add EZ_DECLARE_MESSAGE_TYPE to the type declaration.
/// EZ_IMPLEMENT_MESSAGE_TYPE needs to be added to a cpp.
/// \see ezRTTI
///
/// For the automatic cloning to work and for efficiency the messages must only contain simple data members.
/// For instance, everything that allocates internally (strings, arrays) should be avoided.
/// Instead, such objects should be located somewhere else and the message should only contain pointers to the data.
/// 
class EZ_FOUNDATION_DLL ezMessage : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMessage, ezReflectedClass);

public:
  ezMessage()
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    m_uiDebugMessageRouting = 0;
#endif
  }

  virtual ~ezMessage() {}

  /// \brief Returns a copy of this allocated with the given allocator. This method is automatically implemented by adding EZ_DECLARE_MESSAGE_TYPE.
  virtual ezMessage* Clone(ezAllocatorBase* pAllocator) const = 0;

  /// \brief Derived message types can override this method to influence sorting order. Smaller keys are processed first.
  virtual ezInt32 GetSortingKey() const { return 0; }

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

  /// \brief Implement this for efficient transmission across process boundaries (e.g. network transfer etc.)
  ///
  /// If the message is only ever sent within the same process between nodes of the same ezWorld,
  /// this does not need to be implemented.
  ///
  /// Note that PackageForTransfer() will automatically include the ezRTTI type version into the stream
  /// and ReplicatePackedMessage() will pass this into Deserialize(). Use this if the serialization changes.
  virtual void Serialize(ezStreamWriter& stream) const
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  /// \see Serialize()
  virtual void Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  /// set to true while debugging a message routing problem
  /// if the message is not delivered to any recipient at all, information about why that is will be written to ezLog
  EZ_ALWAYS_INLINE void SetDebugMessageRouting(bool debug)
  {
    m_uiDebugMessageRouting = debug;
  }

  EZ_ALWAYS_INLINE bool GetDebugMessageRouting() const
  {
    return m_uiDebugMessageRouting;
  }
#endif

protected:
  EZ_ALWAYS_INLINE static ezMessageId GetNextMsgId()
  {
    return s_uiNextMsgId++;
  }

  ezMessageId m_Id;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezUInt16 m_uiSize : 15;
  ezUInt16 m_uiDebugMessageRouting : 1;
#else
  ezUInt16 m_uiSize;
#endif

  static ezMessageId s_uiNextMsgId;


  //////////////////////////////////////////////////////////////////////////
  // Transferring and replicating messages
  //

public:
  /// \brief Writes msg to stream in such a way that ReplicatePackedMessage() can restore it even in another process
  ///
  /// For this to work the message type has to have the Serialize and Deserialize functions implemented.
  ///
  /// \note This is NOT used by ezWorld. Within the same process messages can be dispatched more efficiently.
  static void PackageForTransfer(const ezMessage& msg, ezStreamWriter& stream);

  /// \brief Restores a message that was written by PackageForTransfer()
  ///
  /// If the message type is unknown, nullptr is returned.
  /// \see PackageForTransfer()
  ///
  /// \note The returned message has been allocated through the ezRTTI allocator and needs to be manually
  /// deallocated with it again.
  static ezMessage* ReplicatePackedMessage(ezStreamReader& stream);

private:

};

/// \brief Add this macro to the declaration of your custom message type.
#define EZ_DECLARE_MESSAGE_TYPE(messageType, baseType) \
  private: \
    EZ_ADD_DYNAMIC_REFLECTION(messageType, baseType); \
    static ezMessageId MSG_ID; \
  public: \
    static ezMessageId GetTypeMsgId() \
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
  ezMessageId messageType::MSG_ID = messageType::GetTypeMsgId();


/// \brief Base class for all message senders.
template <typename T>
struct ezMessageSenderBase
{
  typedef T MessageType;
};

