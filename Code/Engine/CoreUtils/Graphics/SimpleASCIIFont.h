#pragma once

#include <CoreUtils/Basics.h>
#include <CoreUtils/Image/Image.h>

namespace ezGraphicsUtils
{
  /// \brief Creates a 256*128 RGBA greyscale font texture that contains the 96 most important ASCII characters.
  ///
  /// Each character is embedded in a 16x16 cell, but only uses the center 10x10 pixels, leaving a 3 pixel black boundary.
  /// Each row contains 16 characters, each column contains 8 characters. The characters are sorted in the order in that they
  /// appear in ASCII code, starting at the top left, going to the right and then down. Since the first 32 ASCII characters
  /// are not very useful, the first two rows a left black, except for the very first character, which is an upside down
  /// question mark. This character can be used for unknown characters (e.g. when Utf8 is printed as ASCII and thus characters
  /// above 127 are possible).
  EZ_COREUTILS_DLL void CreateSimpleASCIIFontTexture(ezImage& Img, bool bSetEmptyToUnknown = false);
}

