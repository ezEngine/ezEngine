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

  EZ_FORCE_INLINE ezMessageId GetMessageId() const
  {
    return m_Id;
  }

protected:
  typedef void (*DispatchFunc)(void*, ezMessage&);
  DispatchFunc m_DispatchFunc;
  ezMessageId m_Id;
};


/// \brief [internal] Implements the functionality of ezAbstractMessageHandler.
template <typename Class, typename MessageType, void(Class::*Method)(MessageType&)>
class ezMessageHandler : public ezAbstractMessageHandler
{
public:
  /// \brief Constructor.
  ezMessageHandler()
  {
    m_DispatchFunc = &Dispatch;
    m_Id = MessageType::MSG_ID;
  }

  /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
  static void Dispatch(void* pInstance, ezMessage& msg)
  {
    Class* pTargetInstance = static_cast<Class*>(pInstance);
    (pTargetInstance->*Method)(static_cast<MessageType&>(msg));
  }
};
