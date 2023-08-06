#pragma once

#include <Foundation/Math/Color.h>

/// \brief A color scheme based on https://github.com/yeun/open-color version 1.9.1
///
/// Open Color Goals:
/// All colors will be beautiful in itself and harmonious
/// At the same brightness level, the perceived brightness will be constant
class EZ_FOUNDATION_DLL ezColorScheme
{
public:
  enum Enum
  {
    Red,
    Pink,
    Grape,
    Violet,
    Indigo,
    Blue,
    Cyan,
    Teal,
    Green,
    Lime,
    Yellow,
    Orange,
    Gray,

    Count
  };

  /// \brief Normalization factor for getting colors by index. E.g. ezColorScheme::Blue * s_fIndexNormalizer would get exactly Blue as color.
  constexpr static float s_fIndexNormalizer = 1.0f / (Count - 2);

  /// \brief Get the scheme color with the given brightness (0..9) and with optional saturation and alpha.
  EZ_FORCE_INLINE static ezColor GetColor(Enum schemeColor, ezUInt8 uiBrightness, float fSaturation = 1.0f, float fAlpha = 1.0f)
  {
    EZ_ASSERT_DEV(uiBrightness <= 9, "Brightness is too large");
    const ezColor c = s_Colors[schemeColor][uiBrightness];
    const float l = c.GetLuminance();
    return ezMath::Lerp(ezColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
  }

  /// \brief Get the scheme color using a floating point index instead of the enum. The resulting color will be interpolated between the predefined ones.
  /// Does not include gray.
  static ezColor GetColor(float fIndex, ezUInt8 uiBrightness, float fSaturation = 1.0f, float fAlpha = 1.0f);

  /// \brief Get a scheme color with predefined brightness and saturation to look good with the EZ tools dark UI scheme.
  EZ_ALWAYS_INLINE static ezColor DarkUI(Enum schemeColor)
  {
    return s_DarkUIColors[schemeColor];
  }

  /// \brief Gets a scheme color by index with predefined brightness and saturation to look good with the EZ tools dark UI scheme.
  EZ_FORCE_INLINE static ezColor DarkUI(float fIndex)
  {
    ezUInt32 uiIndexA, uiIndexB;
    float fFrac;
    GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

    return ezMath::Lerp(s_DarkUIColors[uiIndexA], s_DarkUIColors[uiIndexB], fFrac);
  }

  /// \brief Get a scheme color with predefined brightness and saturation to look good as highlight color in EZ tools. Can also be used in a 3D scene for e.g. visualizers etc.
  EZ_ALWAYS_INLINE static ezColor LightUI(Enum schemeColor)
  {
    return s_LightUIColors[schemeColor];
  }

  /// \brief Get a scheme color by index with predefined brightness and saturation to look good as highlight color in EZ tools. Can also be used in a 3D scene for e.g. visualizers etc.
  EZ_FORCE_INLINE static ezColor LightUI(float fIndex)
  {
    ezUInt32 uiIndexA, uiIndexB;
    float fFrac;
    GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

    return ezMath::Lerp(s_LightUIColors[uiIndexA], s_LightUIColors[uiIndexB], fFrac);
  }

  /// \see GetCategoryColor()
  enum class CategoryColorUsage
  {
    ViewportIcon,    // shape icons in 3D viewport
    MenuEntryIcon,   // tint color for icons in a menu
    SceneTreeIcon,   // tint color for icons in a scene tree
    OverlayIcon,     // tint color for overlay icons on top of thumbnails (asset browser)
    BorderColor,     // color for a border frame around UI elements
    BorderIconColor, // color for icons embedded in a border frame
    AssetMenuIcon,   // tint color for icons in asset browser menus
  };

  using CategoryColorFunc = ezColor (*)(ezStringView sCategory, CategoryColorUsage usage);

  static CategoryColorFunc s_CategoryColorFunc;

  /// \brief Returns a color to use in UI for elements of a given 'category'.
  ///
  /// The category is typically defined via an ezCategoryAttribute.
  /// It is simply a string. If it is a complex category, e.g. a path such as "Effects/Wind",
  /// the default implementation only looks at the first part, ie. it treats this all as the category "Effects.
  ///
  /// A custom implementation can be provided through s_CategoryColorFunc, in which case it has full control
  /// and can also use the full category name.
  ///
  /// The 'usage' is provided to tell the function what the color will be used for, allowing to use more or less contrast
  /// or switch of coloring entirely.
  static ezColor GetCategoryColor(ezStringView sCategory, CategoryColorUsage usage);

private:
  EZ_ALWAYS_INLINE constexpr static void GetInterpolation(float fIndex, ezUInt32& out_uiIndexA, ezUInt32& out_uiIndexB, float& out_fFrac)
  {
    fIndex = ezMath::Saturate(fIndex);

    constexpr ezUInt32 uiCountWithoutGray = Count - 1;
    constexpr ezUInt32 uiMaxIndex = uiCountWithoutGray - 1;
    out_uiIndexA = ezUInt32(fIndex * uiMaxIndex);
    out_uiIndexB = (out_uiIndexA + 1) % uiCountWithoutGray;
    out_fFrac = (fIndex * uiMaxIndex) - out_uiIndexA;
  }

  static ezColor s_Colors[Count][10];
  static ezColor s_DarkUIColors[Count];
  static ezColor s_LightUIColors[Count];
};
