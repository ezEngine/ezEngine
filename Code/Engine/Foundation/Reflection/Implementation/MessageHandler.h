#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

class ezMessage;

/// \brief Base class for all message handlers in the reflection system's message dispatch framework.
///
/// Message handlers allow types to declare methods that can receive and process specific message
/// types through the reflection system. This enables loose coupling and dynamic message routing
/// without requiring direct method calls or explicit dependencies.
///
/// The handler system supports both const and non-const member functions, automatically detecting
/// the appropriate calling convention at compile time. Messages are dispatched through type-safe
/// function pointers with minimal overhead.
///
/// This is typically used internally by the reflection macros and should not be used directly.
/// Instead, use EZ_BEGIN_MESSAGEHANDLERS / EZ_MESSAGE_HANDLER / EZ_END_MESSAGEHANDLERS in
/// your reflected type definitions.
class EZ_FOUNDATION_DLL ezAbstractMessageHandler
{
public:
  virtual ~ezAbstractMessageHandler() = default;

  EZ_ALWAYS_INLINE void operator()(void* pInstance, ezMessage& ref_msg) { (*m_DispatchFunc)(this, pInstance, ref_msg); }

  EZ_FORCE_INLINE void operator()(const void* pInstance, ezMessage& ref_msg)
  {
    EZ_ASSERT_DEV(m_bIsConst, "Calling a non const message handler with a const instance.");
    (*m_ConstDispatchFunc)(this, pInstance, ref_msg);
  }

  EZ_ALWAYS_INLINE ezMessageId GetMessageId() const { return m_Id; }

  EZ_ALWAYS_INLINE bool IsConst() const { return m_bIsConst; }

protected:
  using DispatchFunc = void (*)(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage&);
  using ConstDispatchFunc = void (*)(ezAbstractMessageHandler* pSelf, const void* pInstance, ezMessage&);

  union
  {
    DispatchFunc m_DispatchFunc = nullptr;
    ConstDispatchFunc m_ConstDispatchFunc;
  };
  ezMessageId m_Id = ezSmallInvalidIndex;
  bool m_bIsConst = false;
};

struct ezMessageSenderInfo
{
  const char* m_szName;
  const ezRTTI* m_pMessageType;
};

namespace ezInternal
{
  template <typename Class, typename MessageType>
  struct MessageHandlerTraits
  {
    static ezCompileTimeTrueType IsConst(void (Class::*)(MessageType&) const);
    static ezCompileTimeFalseType IsConst(...);
  };

  template <bool bIsConst>
  struct MessageHandler
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&)>
    class Impl : public ezAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_DispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = false;
      }

      static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg)
      {
        EZ_IGNORE_UNUSED(pSelf);
        Class* pTargetInstance = static_cast<Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };

  template <>
  struct MessageHandler<true>
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&) const>
    class Impl : public ezAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_ConstDispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = true;
      }

      /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
      static void Dispatch(ezAbstractMessageHandler* pSelf, const void* pInstance, ezMessage& ref_msg)
      {
        EZ_IGNORE_UNUSED(pSelf);
        const Class* pTargetInstance = static_cast<const Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };
} // namespace ezInternal

#define EZ_IS_CONST_MESSAGE_HANDLER(Class, MessageType, Method) \
  (sizeof(ezInternal::MessageHandlerTraits<Class, MessageType>::IsConst(Method)) == sizeof(ezCompileTimeTrueType))
