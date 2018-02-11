#pragma once

#include <RendererCore/Basics.h>
#include <Core/Messages/ScriptFunctionMessage.h>

struct ezSetColorMode
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    SetAll,
    SetRGB,
    SetAlpha,

    AlphaBlend,
    Additive,
    Modulate,

    Default = SetAll
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSetColorMode);

struct EZ_RENDERERCORE_DLL ezSetColorMessage : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezSetColorMessage, ezScriptFunctionMessage);

  ezColor m_Color;
  ezEnum<ezSetColorMode> m_Mode;

  void ModifyColor(ezColor& color) const;
  void ModifyColor(ezColorGammaUB& color) const;
};
