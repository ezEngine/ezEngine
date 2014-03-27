#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Communication/Message.h>

/// \brief The base class for all message handlers that a type provides.
class EZ_FOUNDATION_DLL ezAbstractMessageHandler
{
public:

  /// \brief Call this function to let an instance handle the given message.
  ///
  /// \note There is NO additional type check, the system will probably cast the given message to the expected message type
  /// and then try to handle that. You need to make sure the type is identical.
  virtual void HandleMessage(void* pInstance, ezMessage* msg) const = 0;

  /// \brief Returns the Message type ID of the type that is handled.
  virtual ezMessageId GetMessageTypeID() const = 0;
};


/// \brief [internal] Implements the functionality of ezAbstractMessageHandler.
template<typename CLASS, typename MESSAGETYPE>
class ezMessageHandler : public ezAbstractMessageHandler
{
public:
  typedef void (CLASS::*TargetFunction)(MESSAGETYPE* msg);

  /// \brief Constructor.
  ezMessageHandler(TargetFunction func)
  {
    m_Function = func;
  }

  /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
  virtual void HandleMessage(void* pInstance, ezMessage* msg) const EZ_OVERRIDE
  {
    CLASS* pTargetInstance = (CLASS*) pInstance;
    (pTargetInstance->*m_Function)((MESSAGETYPE*) msg);
  }

  /// \brief Returns the message type that this handler handles.
  virtual ezMessageId GetMessageTypeID() const EZ_OVERRIDE
  {
    return MESSAGETYPE::MSG_ID;
  }

private:
  TargetFunction m_Function;
};



