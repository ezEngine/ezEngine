#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Communication/Message.h>

/// \brief The base class for all message handlers that a type provides.
class EZ_FOUNDATION_DLL ezAbstractMessageHandler
{
public:
  EZ_FORCE_INLINE void operator()(void* pInstance, ezMessage& msg)
  {
    (*m_DispatchFunc)(pInstance, msg);
  }

  EZ_FORCE_INLINE void operator()(const void* pInstance, ezMessage& msg)
  {
    EZ_ASSERT_DEV(m_bIsConst, "Calling a non const message handler with a const instance.");
    (*m_ConstDispatchFunc)(pInstance, msg);
  }

  EZ_FORCE_INLINE ezMessageId GetMessageId() const
  {
    return m_Id;
  }

  EZ_FORCE_INLINE bool IsConst() const
  {
    return m_bIsConst;
  }

protected:
  typedef void (*DispatchFunc)(void*, ezMessage&);
  typedef void (*ConstDispatchFunc)(const void*, ezMessage&);

  union
  {
    DispatchFunc m_DispatchFunc;
    ConstDispatchFunc m_ConstDispatchFunc;
  };
  ezMessageId m_Id;
  bool m_bIsConst;
};

namespace ezInternal
{
  template <typename Class, typename MessageType>
  struct MessageHandlerTraits
  {
    static ezCompileTimeTrueType IsConst(void(Class::*)(MessageType&) const);
    static ezCompileTimeFalseType IsConst(...);
  };

  template <bool bIsConst>
  struct MessageHandler
  {
    template <typename Class, typename MessageType, void(Class::*Method)(MessageType&)>
    class Impl : public ezAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_DispatchFunc = &Dispatch;
        m_Id = MessageType::GetMsgId();
        m_bIsConst = false;
      }

      static void Dispatch(void* pInstance, ezMessage& msg)
      {
        Class* pTargetInstance = static_cast<Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(msg));
      }
    };
  };

  template <>
  struct MessageHandler<true>
  {
    template <typename Class, typename MessageType, void(Class::*Method)(MessageType&) const>
    class Impl : public ezAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_ConstDispatchFunc = &Dispatch;
        m_Id = MessageType::GetMsgId();
        m_bIsConst = true;
      }

      /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
      static void Dispatch(const void* pInstance, ezMessage& msg)
      {
        const Class* pTargetInstance = static_cast<const Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(msg));
      }
    };
  };
}

#define EZ_IS_CONST_MESSAGE_HANDLER(Class, MessageType, Method) \
  (sizeof(ezInternal::MessageHandlerTraits<Class, MessageType>::IsConst(Method)) == sizeof(ezCompileTimeTrueType))

