#pragma once

#include <RendererCore/Basics.h>
#include <Core/Messages/ScriptFunctionMessage.h>
#include <Core/World/Declarations.h>

struct EZ_RENDERERCORE_DLL ezApplyOnlyToMessage : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezApplyOnlyToMessage, ezScriptFunctionMessage);

  ezGameObjectHandle m_hObject;
};
