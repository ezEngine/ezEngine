#pragma once
#include <Foundation/Types/Types.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Containers/DynamicArray.h>

struct ezFontRenderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Smooth,       /*< Render antialiased fonts without hinting (slightly more blurry). */
    Raster,       /*< Render non-antialiased fonts without hinting (slightly more blurry). */
    HintedSmooth, /*< Render antialiased fonts with hinting. */
    HintedRaster, /*< Render non-antialiased fonts with hinting. */

    Default = Smooth
  };
};

/** Represents a range of character code. */
struct ezCharacterRange
{
  ezCharacterRange() = default;
  ezCharacterRange(ezUInt32 start, ezUInt32 end)
    : m_Start(start)
    , m_End(end)
  {
  }

  ezUInt32 m_Start = 0;
  ezUInt32 m_End = 0;
};

struct ezFontImportOptions
{
  ezFontImportOptions()
  {
    m_FontSizes.PushBack(30);
    m_charIndexRanges.PushBack(ezCharacterRange(0, 166));
  }

  /**	Determines font sizes that are to be imported. Sizes are in points. */
  ezDynamicArray<ezUInt32> m_FontSizes;

  /**	Determines character index ranges to import. Ranges are defined as unicode numbers. */
  ezDynamicArray<ezCharacterRange> m_charIndexRanges; // Most used ASCII characters

  /**	Determines dots per inch scale that will be used when rendering the characters. */
  ezUInt32 m_Dpi = 96;

  /**	Determines the render mode used for rendering the characters into a bitmap. */
  ezEnum<ezFontRenderMode> m_RenderMode;

  /**	Determines whether the bold font style should be used when rendering. */
  bool m_Bold = false;

  /**	Determines whether the italic font style should be used when rendering. */
  bool m_Italic = false;
};
