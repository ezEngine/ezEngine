#pragma once

#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezDeleteObjectMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezDeleteObjectMessage);
};

