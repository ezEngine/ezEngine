#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Math.h>

/// \brief A 8bit per channel color storage format with undefined encoding. It is up to the user to reinterpret as a gamma or linear space
/// color.
///
/// \see ezColorLinearUB
/// \see ezColorGammaUB
class EZ_FOUNDATION_DLL ezColorBaseUB
{
public:
  EZ_DECLARE_POD_TYPE();

  ezUInt8 r;
  ezUInt8 g;
  ezUInt8 b;
  ezUInt8 a;

  /// \brief Default-constructed color is uninitialized (for speed)
  ezColorBaseUB() = default;

  /// \brief Initializes the color with r, g, b, a
  ezColorBaseUB(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a = 255);

  /// \brief Conversion to const ezUInt8*.
  const ezUInt8* GetData() const { return &r; }

  /// \brief Conversion to ezUInt8*
  ezUInt8* GetData() { return &r; }

  /// \brief Packs the 4 color values into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] ezUInt32 ToRGBA8() const
  {
    // RGBA (A at lowest address, R at highest)
    return (static_cast<ezUInt32>(r) << 24) +
           (static_cast<ezUInt32>(g) << 16) +
           (static_cast<ezUInt32>(b) << 8) +
           (static_cast<ezUInt32>(a) << 0);
  }

  /// \brief Packs the 4 color values into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] ezUInt32 ToABGR8() const
  {
    // RGBA (A at highest address, R at lowest)
    return (static_cast<ezUInt32>(a) << 24) +
           (static_cast<ezUInt32>(b) << 16) +
           (static_cast<ezUInt32>(g) << 8) +
           (static_cast<ezUInt32>(r) << 0);
  }
};

static_assert(sizeof(ezColorBaseUB) == 4);

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in linear space.
///
/// For any calculations or conversions use ezColor.
/// \see ezColor
class EZ_FOUNDATION_DLL ezColorLinearUB : public ezColorBaseUB
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default-constructed color is uninitialized (for speed)
  ezColorLinearUB() = default; // [tested]

  /// \brief Initializes the color with r, g, b, a
  ezColorLinearUB(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a = 255); // [tested]

  /// \brief Initializes the color with ezColor.
  /// Assumes that the given color is normalized.
  /// \see ezColor::IsNormalized
  ezColorLinearUB(const ezColor& color); // [tested]

  /// \brief Initializes the color with ezColor.
  void operator=(const ezColor& color); // [tested]

  /// \brief Converts this color to ezColor.
  ezColor ToLinearFloat() const; // [tested]

  /// \brief Extracts the values from a uint32 with R at the least significant bits, then G, then B and A at the most significant bits.
  static ezColorLinearUB MakeFromABGR8(ezUInt32 value)
  {
    return ezColorLinearUB(static_cast<ezUInt8>(value >> 0) & 0xFF,
      static_cast<ezUInt8>(value >> 8) & 0xFF,
      static_cast<ezUInt8>(value >> 16) & 0xFF,
      static_cast<ezUInt8>(value >> 24) & 0xFF);
  }
};

static_assert(sizeof(ezColorLinearUB) == 4);

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in gamma space.
///
/// For any calculations or conversions use ezColor.
/// \see ezColor
class EZ_FOUNDATION_DLL ezColorGammaUB : public ezColorBaseUB
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default-constructed color is uninitialized (for speed)
  ezColorGammaUB() = default;

  /// \brief Copies the color values. RGB are assumed to be in Gamma space.
  ezColorGammaUB(ezUInt8 uiGammaRed, ezUInt8 uiGammaGreen, ezUInt8 uiGammaBlue, ezUInt8 uiLinearAlpha = 255); // [tested]

  /// \brief Initializes the color with ezColor. Converts the linear space color to gamma space.
  /// Assumes that the given color is normalized.
  /// \see ezColor::IsNormalized
  ezColorGammaUB(const ezColor& color); // [tested]

  /// \brief Initializes the color with ezColor. Converts the linear space color to gamma space.
  void operator=(const ezColor& color); // [tested]

  /// \brief Converts this color to ezColor.
  ezColor ToLinearFloat() const;
};

static_assert(sizeof(ezColorGammaUB) == 4);


#include <Foundation/Math/Implementation/Color8UNorm_inl.h>
