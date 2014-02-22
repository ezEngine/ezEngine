#pragma once

#include <CoreUtils/Image/ImageFormat.h>

/// \brief Helper class containing methods to convert between ezImageFormat::Enum and platform-specific image formats.
class EZ_COREUTILS_DLL ezImageFormatMappings
{
public:
  /// \brief Maps an ezImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static ezUInt32 ToDxgiFormat(ezImageFormat::Enum format);

  /// \brief Maps a Direct3D DXGI_FORMAT to an equivalent ezImageFormat::Enum.
  static ezImageFormat::Enum FromDxgiFormat(ezUInt32 uiDxgiFormat);

  /// \brief Maps an ezImageFormat::Enum to an equivalent FourCC code.
  static ezUInt32 ToFourCc(ezImageFormat::Enum format);

  /// \brief Maps a FourCC code to an equivalent ezImageFormat::Enum.
  static ezImageFormat::Enum FromFourCc(ezUInt32 uiFourCc);
};