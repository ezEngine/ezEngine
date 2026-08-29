#pragma once

#include <Texture/Image/ImageFormat.h>

/// Helper class containing methods to convert between ezImageFormat::Enum and platform-specific image formats.
class EZ_TEXTURE_DLL ezImageFormatMappings
{
public:
  /// Maps an ezImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static ezUInt32 ToDxgiFormat(ezImageFormat::Enum format);

  /// Maps a Direct3D DXGI_FORMAT to an equivalent ezImageFormat::Enum.
  static ezImageFormat::Enum FromDxgiFormat(ezUInt32 uiDxgiFormat);

  /// Maps an ezImageFormat::Enum to an equivalent FourCC code.
  static ezUInt32 ToFourCc(ezImageFormat::Enum format);

  /// Maps a FourCC code to an equivalent ezImageFormat::Enum.
  static ezImageFormat::Enum FromFourCc(ezUInt32 uiFourCc);
};
