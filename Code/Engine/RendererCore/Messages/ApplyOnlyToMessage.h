#pragma once

#include <RendererCore/Basics.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct EZ_RENDERERCORE_DLL ezMsgOnlyApplyToObject : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgOnlyApplyToObject, ezMessage);

  ezGameObjectHandle m_hObject;
};

