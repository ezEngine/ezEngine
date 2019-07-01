#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezMsgTransformChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTransformChanged, ezMessage);

  ezTransform m_OldGlobalTransform;
  ezTransform m_NewGlobalTransform;
};
