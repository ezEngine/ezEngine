#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Communication/Message.h>


class ezAbstractMessageHandler
{
public:
  virtual void HandleMessage(void* pInstance, ezMessage* msg) const = 0;

  /// \brief Returns the Message type ID of the type that is handled.
  virtual ezMessageId GetMessageTypeID() const = 0;
};


template<typename CLASS, typename MESSAGETYPE>
class ezMessageHandler : public ezAbstractMessageHandler
{
public:
  typedef void (CLASS::*TargetFunction)(MESSAGETYPE* msg);

  ezMessageHandler(TargetFunction func)
  {
    m_Function = func;
  }

  virtual void HandleMessage(void* pInstance, ezMessage* msg) const EZ_OVERRIDE
  {
    CLASS* pTargetInstance = (CLASS*) pInstance;
    (pTargetInstance->*m_Function)((MESSAGETYPE*) msg);
  }

  virtual ezMessageId GetMessageTypeID() const EZ_OVERRIDE
  {
    return MESSAGETYPE::MSG_ID;
  }

private:
  TargetFunction m_Function;
};



