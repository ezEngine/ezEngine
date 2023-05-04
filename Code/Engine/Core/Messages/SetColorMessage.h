#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct ezSetColorMode
{
  using StorageType = ezUInt32;

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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezSetColorMode);

struct EZ_CORE_DLL ezMsgSetColor : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetColor, ezMessage);

  ezColor m_Color;
  ezEnum<ezSetColorMode> m_Mode;

  void ModifyColor(ezColor& ref_color) const;
  void ModifyColor(ezColorGammaUB& ref_color) const;

  //////////////////////////////////////////////////////////////////////////
  // ezMessage interface
  //

  virtual void Serialize(ezStreamWriter& inout_stream) const override;
  virtual void Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion) override;
};
