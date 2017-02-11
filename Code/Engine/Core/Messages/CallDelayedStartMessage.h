#pragma once

#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezCallDelayedStartMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezCallDelayedStartMessage);

  // make sure this messages is processed before all others
  virtual ezInt32 GetSortingKey() const override 
  {
    return (-2147483647 - 1);
  }
};

