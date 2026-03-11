#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

/// \brief Describes how a color should be applied to another color.
struct ezSetColorMode
{
  using StorageType = ezUInt32;

  enum Enum
  {
    SetRGBA,    ///< Overrides all four RGBA values.
    SetRGB,     ///< Overrides the RGB values but leaves Alpha untouched.
    SetAlpha,   ///< Overrides Alpha, leaves RGB untouched.

    AlphaBlend, ///< Modifies the target RGBA values by interpolating from the previous color towards the incoming color using the incoming alpha value.
    Additive,   ///< Adds to the RGBA values.
    Modulate,   /// Multiplies the RGBA values.

    Default = SetRGBA
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezSetColorMode);

/// \brief A message to modify the main color of some thing.
///
/// Components that handle this message use it to change their main color.
/// For instance a light component may change its light color, a mesh component will change the main mesh color.
struct EZ_CORE_DLL ezMsgSetColor : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetColor, ezMessage);

  /// \brief The color to apply to the target.
  ezColor m_Color;

  /// \brief The mode with which to apply the color to the target.
  ezEnum<ezSetColorMode> m_Mode;

  /// \brief Applies m_Color using m_Mode to the given color.
  void ModifyColor(ezColor& ref_color) const;

  /// \brief Applies m_Color using m_Mode to the given color.
  void ModifyColor(ezColorGammaUB& ref_color) const;

  virtual void Serialize(ezStreamWriter& inout_stream) const override;
  virtual void Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion) override;
};

/// \brief Message to set custom float data on components.
///
/// Provides four float values that can be used to pass arbitrary data to components.
/// The interpretation of these values depends on the receiving component.
struct EZ_CORE_DLL ezMsgSetCustomData : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetCustomData, ezMessage);

  float m_fData0;
  float m_fData1;
  float m_fData2;
  float m_fData3;

  virtual void Serialize(ezStreamWriter& inout_stream) const override;
  virtual void Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion) override;
};
