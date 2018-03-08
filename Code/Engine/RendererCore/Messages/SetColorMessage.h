#pragma once

#include <RendererCore/Basics.h>

struct ezSetColorMode
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    SetRGBA,
    SetRGB,
    SetAlpha,

    AlphaBlend,
    Additive,
    Modulate,

    Default = SetRGBA
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSetColorMode);

struct EZ_RENDERERCORE_DLL ezMsgSetColor : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetColor, ezMessage);

  ezColor m_Color;
  ezEnum<ezSetColorMode> m_Mode;

  void ModifyColor(ezColor& color) const;
  void ModifyColor(ezColorGammaUB& color) const;

  //////////////////////////////////////////////////////////////////////////
  // ezMessage interface
  //

  virtual void Serialize(ezStreamWriter& stream) const override;
  virtual void Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion) override;
};
