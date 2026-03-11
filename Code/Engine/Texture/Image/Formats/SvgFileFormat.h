#pragma once

#ifdef BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT

#  include <Texture/Image/Formats/ImageFileFormat.h>

/// SVG file format support using LunaSVG.
///
/// Rasterizes SVG files to RGBA images.
/// Target resolution and background color can be adjusted on the loader.
///
/// Note: Instantiate the loader directly to override the defaults.
class EZ_TEXTURE_DLL ezSvgFileFormat : public ezImageFileFormat
{
public:
  /// The resolution at which to rasterize the image.
  ezUInt32 m_uiResolutionX = 512;
  ezUInt32 m_uiResolutionY = 512;
  ezColorGammaUB m_Background{0, 0, 0, 0}; // transparent

  ezResult ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const override;
  ezResult ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const override;
  ezResult WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const override;

  bool CanReadFileType(ezStringView sExtension) const override;
  bool CanWriteFileType(ezStringView sExtension) const override;
};

#endif
